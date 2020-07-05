import("stdfaust.lib");

numBands = 5;
rangeDb = 12.0;
lokey = 48; // C3
hikey = 108; // C8

l(i) = hslider("[%i] EQ Band %j [unit:%]", 50, 0, 100, 1) : pcTodB with { j = i+1; };
pcTodB = *(rangeDb*2.0/100.0)-rangeDb;

f(i) = ba.midikey2hz(lokey+(hikey-lokey)*i*(1.0/(numBands-1)));
b(i) = 0.5*(f(i+1)-f(i));

eq = fi.low_shelf(l(0), f(0)) :
     seq(i, numBands-2, fi.peak_eq(l(i+1), f(i+1), b(i+1))) :
     fi.high_shelf(l(numBands-1), f(numBands-1));

process = (eq, eq); // : co.limiter_1176_R4_stereo;
