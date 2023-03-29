// Copyright (c) 2003-2016  Pavel Rychly, Milos Jakubicek

#include "seek.hh"
#include "excep.hh"
#include "consumer.hh"
#include "bititer.hh"
#include "bitio.hh"
#include "fromtof.hh"
#include "binfile.hh"
#include "fstream.hh"
#include <limits>
#include <stdexcept>

using namespace std;


class write_segfile {
public:
    virtual void write (off_t bytes, int bits) =0;
    virtual ~write_segfile () noexcept(false) {}
    virtual void get_last (off_t &bytes, int &bits) =0;
};

class write_oneseg : public write_segfile {
    string path;
protected:
    FILE *segf;
    ToFile<uint32_t> seg;
public:
    write_oneseg (const string &filename, bool append=false)
        : path (filename), segf (fopen ((filename + ".seg").c_str(), (append ? "rb+" : "wb"))),
        seg (segf) {}
    virtual ~write_oneseg () {fclose (segf);}
    virtual void write (off_t bytes, int bits) {
        if (8 * bytes + bits > numeric_limits<uint32_t>::max()) {
            throw overflow_error ("File too large for FD_FD, use FD_FGD");
        }
        seg (8 * bytes + bits);
    }
    virtual void get_last (off_t &bytes, int &bits) {
        uint32_t storedbits;
        fseeko (segf, -4, SEEK_END);
        if (fread (&storedbits, sizeof (storedbits), 1 , segf) != 1) {
            throw runtime_error("write_oneseg::get_last(): fread failed");
        }
        fseeko (segf, -4, SEEK_END);
        bytes = storedbits / 8;
        bits = storedbits % 8;
    }
};

class write_bigseg : public write_oneseg {
protected:
    FILE *seg2f;
    ToFile<int8_t> seg2;
public:
    write_bigseg (const string &filename, bool append=false)
        : write_oneseg (filename, append), 
        seg2f (fopen ((filename + ".seg2").c_str(), (append ? "rb+" : "wb"))), 
        seg2 (seg2f) {}
    virtual ~write_bigseg () {fclose (seg2f);}
    virtual void write (off_t bytes, int bits) {
        if (bytes > numeric_limits<uint32_t>::max()) {
            throw overflow_error ("File too large for FD_FBD, use FD_FGD");
        }
        seg (bytes);
        seg2 (bits);
    }
    virtual void get_last (off_t &bytes, int &bits) {
        uint32_t storedbytes;
        fseeko (segf, -4, SEEK_END);
        if (fread (&storedbytes, sizeof (storedbytes), 1 , segf) != 1) {
            throw runtime_error("write_bigseg::get_last(): fread failed");
        }
        fseeko (segf, -4, SEEK_END);
        bytes = storedbytes;
        int8_t storedbits;
        fseeko (seg2f, -1, SEEK_END);
        if (fread (&storedbits, sizeof (storedbits), 1 , seg2f) != 1) {
            throw runtime_error("write_bigseg::get_last(): fread failed");
        }
        fseeko (seg2f, -1, SEEK_END);
        bits = storedbits;
    }
};

class write_gigaseg : public write_segfile {
protected:
    FILE *offf;
    FILE *segf;
    ToFile<uint16_t> offsets;
    ToFile<uint32_t> segments;
    off_t lastseg;
    int segnum;
public:
    write_gigaseg (const string &filename, bool append=false)
        : offf (fopen ((filename + ".off").c_str(), (append ? "rb+" : "wb"))), 
          segf (fopen ((filename + ".seg").c_str(), (append ? "rb+" : "wb"))), 
          offsets (offf), segments (segf), lastseg (0), segnum (0) 
    {}
    virtual ~write_gigaseg () {fclose (segf); fclose (offf);}
    virtual void write (off_t bytes, int bits) {
        if (segnum % 16 == 0) {
            segnum = 0;
            lastseg = bytes / 2048;
            segments (lastseg);
            lastseg *= 2048;
        }
        segnum++;
        bytes -= lastseg;
        offsets (8 * bytes + bits);
    }
    virtual void get_last (off_t &bytes, int &bits) {
        uint16_t last_off_val;
        uint32_t last_seg_val;
        fseeko (offf, -2, SEEK_END);
        if (fread (&last_off_val, sizeof (last_off_val), 1 , offf) != 1) {
            throw runtime_error("write_gigaseg::get_last(): fread failed");
        }
        fseeko (offf, -2, SEEK_END);
        segnum = ftello (offf) / sizeof(uint16_t) % 16;
        fseeko (segf, -4, SEEK_END);
        if (fread (&last_seg_val, sizeof (last_seg_val), 1 , segf) != 1) {
            throw runtime_error("write_gigaseg::get_last(): fread failed");
        }
        if (segnum == 0)
            fseeko (segf, -4, SEEK_END);
        lastseg = off_t(last_seg_val) * 2048;
        bits = last_off_val % 8;
        bytes = lastseg + last_off_val / 8;
    }
};

class DeltaTextConsumer: public TextConsumer {
protected:
    typedef write_bits<OutFileBits_tell&,unsigned char,unsigned char*> Writer;
    int seg_size;
    bool append_to_end;
    FILE *textf;
    Position text_size;
    OutFileBits_tell *deltafile;
    Writer *delta;
    static const char signature[7];
    write_segfile *seg;
    string path;
public:
    DeltaTextConsumer (const string &path, int seg_size, write_segfile *seg,
                       bool append)
        : seg_size (seg_size), append_to_end (append),
          textf (fopen (path.c_str(), (append_to_end ? "rb+" : "wb"))),
          text_size (0), seg (seg), path (path)
    {
        if (!textf) throw FileAccessError (path, "DeltaTextConsumer()");
        if (append_to_end) {
            // read stored parameters
            {
                BinFile<unsigned char>::const_iterator t (textf, 16,
                                                          path.c_str());
                read_bits<BinFile<unsigned char>::const_iterator,
                    unsigned char, Position> rb (t);
                seg_size = rb.delta() -1;
                text_size = rb.delta() -1;
            }
            // locate the end
            off_t bytes;
            int skip_bits = 0;
            unsigned char last_byte;
            seg->get_last (bytes, skip_bits);
            fseeko (textf, -1, SEEK_END);
            if (fread (&last_byte, 1, 1, textf) != 1) {
                throw FileAccessError(path, "DeltaTextConsumer(): fread");
            }
            fseeko (textf, bytes, SEEK_SET);
            deltafile = new OutFileBits_tell(textf);
            delta = new Writer (*deltafile);
            if (skip_bits > 0)
                delta->load (&last_byte, skip_bits);
        } else {
            deltafile = new OutFileBits_tell(textf);
            // skip pool for header, seg_size and text_size
            const int header[] = {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                // text_size=0
                0x88, 0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            for (int i=0; i < 32; i++, ++(*deltafile))
                *(*deltafile) = header[i];
            delta = new Writer (*deltafile);
        }
    }
    virtual ~DeltaTextConsumer() noexcept(false)
    {
        seg->write (deltafile->tell(), delta->bitslen());
        delete delta;
        delete seg;

        if (!append_to_end) {
            // write header
            fseeko (textf, 0, SEEK_SET);
            if (fwrite (signature, sizeof (signature) -1, 1, textf) != 1) {
                throw FileAccessError(path, "~DeltaTextConsumer(): fwrite");
            }
        }

        // write seg_size and text_size
        fseeko (textf, 16, SEEK_SET);
        {
            write_bits<OutFileBits_tell&,unsigned char,unsigned char*,Position> 
                write_size(*deltafile);
            write_size.delta (seg_size +1);
            write_size.delta (text_size +1);
            //msg ("header written, seg: %i, size: %i", seg_size, text_size);
        }
        delete deltafile;
        fclose (textf);
    }
    virtual void put (int id) 
    {
        if (text_size % seg_size == 0)
            seg->write (deltafile->tell(), delta->bitslen());
        delta->delta (id +1);
        text_size++;
    }
    virtual Position curr_size()
    {
        return text_size;
    }
};      

const char DeltaTextConsumer::signature[7] = "\243finDT";

class IntTextConsumer: public TextConsumer {
protected:
    static const char signature[16];
    FILE *textf;
    string path;
public:
    IntTextConsumer (const string &path, bool append=false)
        : textf (fopen (path.c_str(), (append ? "rb+" : "wb"))), path (path)
    {
        if (!textf) throw FileAccessError (path, "IntTextConsumer");
        if (append)
            fseeko (textf, 0, SEEK_END);
        else {
            if (fwrite (signature, 16, 1, textf) != 1) {
                throw FileAccessError (path, "IntTextConsumer(): fwrite");
            }
        }
    }
    virtual ~IntTextConsumer() {
        fclose (textf);
    }
    virtual void put (int id) {
        int32_t idt = id;
        if (fwrite (&idt, sizeof (int32_t), 1, textf) != 1) {
            throw FileAccessError (path, "~IntTextConsumer(): fwrite");
        }
    }
    virtual Position curr_size()
    {
        return (ftello(textf) - 16) / sizeof(int32_t);
    }
};

const char IntTextConsumer::signature[16] = "\243finIT\0\0\0\0\0\0\0\0\0";


TextConsumer* TextConsumer::create (TextConsumer::TextType tt, 
                                    const string &path, bool append)
{
    write_segfile *seg;
    TextConsumer *tc;
    int segsize = 128;
    switch (tt) {
    case tdeltagiga:
        seg = new write_gigaseg (path + ".text", append);
        segsize = 64;
        break;
    case tdeltabig:
        seg = new write_bigseg (path + ".text", append);
        break;
    case tdelta:
        seg = new write_oneseg (path + ".text", append);
        break;
    case tint:
        tc = new IntTextConsumer (path + ".text", append);
        tc->type = tt;
        return tc;
    default:
        return NULL;
    }
    tc = new DeltaTextConsumer (path + ".text", segsize, seg, append);
    tc->type = tt;
    return tc;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
