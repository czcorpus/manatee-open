g++ -o lua_callback_demo lua_callback_demo.cc     -I.. -I../concord -I../corp -I../query -I../finlib -I../hat-trie/src     -I/usr/include/lua5.3     ../concord/.libs/libconcord.a     ../corp/.libs/libcorp.a     ../query/.libs/libquery.a     ../finlib/.libs/libfin.a     /usr/local/lib/libhat-trie.a     ../fsa3/.libs/libfsa3.a     -llua5.3 -lpcre -pthread


