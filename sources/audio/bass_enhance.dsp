import("stdfaust.lib");

driveMax = 0.5;
toneMin = 50.0;
toneMax = 600.0;

drive = hslider("[0] Drive [unit:%]", 0.0, 0.0, 100.0, 0.01) : *(0.01*driveMax);
tone = hslider("[1] Tone [unit:%]", 0.0, 0.0, 100.0, 0.01) : *(0.01*(toneMax-toneMin)) : +(toneMin);

phatbass = _ <: ((lows : distort), highs) :> + with {
  split = fi.filterbank(3, (tone));
  lows = split : (!, _);
  highs = split : (_, !);
  distort = ef.cubicnl(drive, 0.0);
};

process = (phatbass, phatbass);
