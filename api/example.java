// example.java

import java.io.*;
import com.phrasebox.manatee.*;

public class example {
  static {
      //System.loadLibrary("manatee");
      System.load("/home/pary/src/manatee/api/.libs/libmanatee.so");
  }

    public static void browse_corp (PosAttr attr, int begpos, int endpos) {
	int pos = begpos;
	if (pos < 0) 
	    pos = 0;
	TextIterator iter = attr.textat (pos);
	while (pos < endpos) {
	    pos ++;
	    System.out.print(iter.next());
	    System.out.print(' ');
	}
	System.out.println();
    }
    
    public static String strip_tags (StrVector tokens) {
	String ret="";
	for (int i=0;  i < tokens.size(); i+=2) {
	    ret += tokens.get(i);
	    ret += ' ';
	}
	return ret;
    }

  public static void main(String argv[]) {
    System.out.println(manatee.version());


    Corpus corp = new Corpus("susanne");
    PosAttr wordattr = corp.get_attr ("word");
    System.out.println(corp.size());
    
    if (argv.length == 0) {
	System.out.println ("usage: example QUERY");
	return;
    }
    Concordance conc = new Concordance();
    conc.load_from_query (corp, argv[0], 0, 0);
    System.out.println("result:" + conc.size());
    int maxline = 15;
    if (maxline > conc.size())
	maxline = conc.size();
    KWICLines kl = new KWICLines (conc, "15#", "15#", 
				  "word", "word", "s", "#", 100, false);
    for (int linenum = 0; linenum < maxline; linenum++) {
	if (! kl.nextline (linenum))
	    break;
	System.out.printf ("%s %s <%s> %s\n",kl.get_refs(), 
			   strip_tags (kl.get_left()),
			   strip_tags (kl.get_kwic()),
			   strip_tags (kl.get_right()));
	
    }
    
    if (argv.length > 1) {
	StrVector words = new StrVector();
	IntVector freqs = new IntVector();
	IntVector norms = new IntVector();
	conc.freq_dist ("word 1 lemma 1", Integer.parseInt(argv[1]), 
			words, freqs, norms);
	for (int i=0; i < words.size(); i++)
	    System.out.printf ("%s\t%d\n", words.get(i), freqs.get(i));

	System.out.println();
	CollocItems coll = new CollocItems (conc, "word", 'f', 1,1, -5, 5, 20);
	while (!coll.eos()) {
	    System.out.printf ("%s\t%d\t%d\n", coll.get_item(), coll.get_freq(), coll.get_cnt());
	    coll.next();
	}
    }
  }
}
