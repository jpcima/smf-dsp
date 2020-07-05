/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "fverb"
version: "0.5"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __Reverb_dsp_H__
#define  __Reverb_dsp_H__

#include "audio/wrap/faust.h"


#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>

class Reverb_dspSIG0 {
	
  private:
	
	int iRec15[2];
	
  public:
	
	int getNumInputsReverb_dspSIG0() {
		return 0;
	}
	int getNumOutputsReverb_dspSIG0() {
		return 1;
	}
	int getInputRateReverb_dspSIG0(int channel) {
		int rate;
		switch ((channel)) {
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	int getOutputRateReverb_dspSIG0(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 0;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	
	void instanceInitReverb_dspSIG0(int sample_rate) {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			iRec15[l0] = 0;
		}
	}
	
	void fillReverb_dspSIG0(int count, float* table) {
		for (int i = 0; (i < count); i = (i + 1)) {
			iRec15[0] = (iRec15[1] + 1);
			table[i] = std::sin((9.58738019e-05f * float((iRec15[0] + -1))));
			iRec15[1] = iRec15[0];
		}
	}

};

static Reverb_dspSIG0* newReverb_dspSIG0() { return (Reverb_dspSIG0*)new Reverb_dspSIG0(); }
static void deleteReverb_dspSIG0(Reverb_dspSIG0* dsp) { delete dsp; }

static float ftbl0Reverb_dspSIG0[65536];

#ifndef FAUSTCLASS 
#define FAUSTCLASS Reverb_dsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class Reverb_dsp : public dsp {
	
 private:
	
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec16[2];
	float fConst2;
	float fConst3;
	float fRec11[2];
	float fRec12[2];
	int iRec13[2];
	int iRec14[2];
	float fConst4;
	float fRec27[2];
	float fRec26[2];
	float fConst5;
	float fRec28[2];
	float fRec25[2];
	int IOTA;
	float fVec0[1024];
	int iConst6;
	float fRec23[2];
	float fVec1[1024];
	int iConst7;
	float fRec21[2];
	float fVec2[4096];
	int iConst8;
	float fRec19[2];
	float fVec3[2048];
	int iConst9;
	float fRec17[2];
	int iConst10;
	float fVec4[131072];
	float fRec9[2];
	float fVec5[32768];
	int iConst11;
	float fConst12;
	float fRec29[2];
	float fRec8[2];
	float fVec6[32768];
	int iConst13;
	float fRec6[2];
	float fRec0[32768];
	float fRec1[16384];
	float fRec2[32768];
	float fRec35[2];
	float fRec36[2];
	int iRec37[2];
	int iRec38[2];
	float fRec48[2];
	float fRec47[2];
	float fVec7[1024];
	int iConst14;
	float fRec45[2];
	float fVec8[1024];
	int iConst15;
	float fRec43[2];
	float fVec9[4096];
	int iConst16;
	float fRec41[2];
	float fVec10[2048];
	int iConst17;
	float fRec39[2];
	int iConst18;
	float fVec11[131072];
	float fRec33[2];
	float fVec12[32768];
	int iConst19;
	float fRec32[2];
	float fVec13[16384];
	int iConst20;
	float fRec30[2];
	float fRec3[32768];
	float fRec4[8192];
	float fRec5[32768];
	int iConst21;
	int iConst22;
	int iConst23;
	int iConst24;
	int iConst25;
	int iConst26;
	int iConst27;
	int iConst28;
	int iConst29;
	int iConst30;
	int iConst31;
	int iConst32;
	int iConst33;
	int iConst34;
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("author", "Jean Pierre Cimalando");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.1");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "0.1");
		m->declare("filename", "reverb.dsp");
		m->declare("filters.lib/allpass_comb:author", "Julius O. Smith III");
		m->declare("filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("license", "BSD-2-Clause");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.1");
		m->declare("name", "fverb");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "0.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "0.0");
		m->declare("version", "0.5");
	}

	virtual int getNumInputs() {
		return 2;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	virtual int getInputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	virtual int getOutputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	
	static void classInit(int sample_rate) {
		Reverb_dspSIG0* sig0 = newReverb_dspSIG0();
		sig0->instanceInitReverb_dspSIG0(sample_rate);
		sig0->fillReverb_dspSIG0(65536, ftbl0Reverb_dspSIG0);
		deleteReverb_dspSIG0(sig0);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = (1.0f / fConst0);
		fConst2 = (1.0f / float(int((0.00999999978f * fConst0))));
		fConst3 = (0.0f - fConst2);
		fConst4 = (0.00100000005f * std::exp((0.0f - (62831.8516f / fConst0))));
		fConst5 = (0.00100000005f * std::exp((0.0f - (628.318542f / fConst0))));
		iConst6 = std::min<int>(65536, std::max<int>(0, (int((0.00462820474f * fConst0)) + -1)));
		iConst7 = std::min<int>(65536, std::max<int>(0, (int((0.00370316859f * fConst0)) + -1)));
		iConst8 = std::min<int>(65536, std::max<int>(0, (int((0.013116831f * fConst0)) + -1)));
		iConst9 = std::min<int>(65536, std::max<int>(0, (int((0.00902825873f * fConst0)) + -1)));
		iConst10 = (std::min<int>(65536, std::max<int>(0, int((0.106280029f * fConst0)))) + 1);
		iConst11 = std::min<int>(65536, std::max<int>(0, int((0.141695514f * fConst0))));
		fConst12 = (0.00100000005f * std::exp((0.0f - (34557.5195f / fConst0))));
		iConst13 = std::min<int>(65536, std::max<int>(0, (int((0.0892443135f * fConst0)) + -1)));
		iConst14 = std::min<int>(65536, std::max<int>(0, (int((0.00491448538f * fConst0)) + -1)));
		iConst15 = std::min<int>(65536, std::max<int>(0, (int((0.00348745007f * fConst0)) + -1)));
		iConst16 = std::min<int>(65536, std::max<int>(0, (int((0.0123527432f * fConst0)) + -1)));
		iConst17 = std::min<int>(65536, std::max<int>(0, (int((0.00958670769f * fConst0)) + -1)));
		iConst18 = (std::min<int>(65536, std::max<int>(0, int((0.124995798f * fConst0)))) + 1);
		iConst19 = std::min<int>(65536, std::max<int>(0, int((0.149625346f * fConst0))));
		iConst20 = std::min<int>(65536, std::max<int>(0, (int((0.0604818389f * fConst0)) + -1)));
		iConst21 = std::min<int>(65536, std::max<int>(0, int((0.00893787201f * fConst0))));
		iConst22 = std::min<int>(65536, std::max<int>(0, int((0.099929437f * fConst0))));
		iConst23 = std::min<int>(65536, std::max<int>(0, int((0.067067638f * fConst0))));
		iConst24 = std::min<int>(65536, std::max<int>(0, int((0.0642787516f * fConst0))));
		iConst25 = std::min<int>(65536, std::max<int>(0, int((0.0668660328f * fConst0))));
		iConst26 = std::min<int>(65536, std::max<int>(0, int((0.0062833908f * fConst0))));
		iConst27 = std::min<int>(65536, std::max<int>(0, int((0.0358186886f * fConst0))));
		iConst28 = std::min<int>(65536, std::max<int>(0, int((0.0118611604f * fConst0))));
		iConst29 = std::min<int>(65536, std::max<int>(0, int((0.121870905f * fConst0))));
		iConst30 = std::min<int>(65536, std::max<int>(0, int((0.0898155272f * fConst0))));
		iConst31 = std::min<int>(65536, std::max<int>(0, int((0.041262053f * fConst0))));
		iConst32 = std::min<int>(65536, std::max<int>(0, int((0.070931755f * fConst0))));
		iConst33 = std::min<int>(65536, std::max<int>(0, int((0.0112563418f * fConst0))));
		iConst34 = std::min<int>(65536, std::max<int>(0, int((0.00406572362f * fConst0))));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(50.0f);
		fHslider1 = FAUSTFLOAT(50.0f);
	}
	
	virtual void instanceClear() {
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec16[l1] = 0.0f;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec11[l2] = 0.0f;
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec12[l3] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			iRec13[l4] = 0;
		}
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			iRec14[l5] = 0;
		}
		for (int l6 = 0; (l6 < 2); l6 = (l6 + 1)) {
			fRec27[l6] = 0.0f;
		}
		for (int l7 = 0; (l7 < 2); l7 = (l7 + 1)) {
			fRec26[l7] = 0.0f;
		}
		for (int l8 = 0; (l8 < 2); l8 = (l8 + 1)) {
			fRec28[l8] = 0.0f;
		}
		for (int l9 = 0; (l9 < 2); l9 = (l9 + 1)) {
			fRec25[l9] = 0.0f;
		}
		IOTA = 0;
		for (int l10 = 0; (l10 < 1024); l10 = (l10 + 1)) {
			fVec0[l10] = 0.0f;
		}
		for (int l11 = 0; (l11 < 2); l11 = (l11 + 1)) {
			fRec23[l11] = 0.0f;
		}
		for (int l12 = 0; (l12 < 1024); l12 = (l12 + 1)) {
			fVec1[l12] = 0.0f;
		}
		for (int l13 = 0; (l13 < 2); l13 = (l13 + 1)) {
			fRec21[l13] = 0.0f;
		}
		for (int l14 = 0; (l14 < 4096); l14 = (l14 + 1)) {
			fVec2[l14] = 0.0f;
		}
		for (int l15 = 0; (l15 < 2); l15 = (l15 + 1)) {
			fRec19[l15] = 0.0f;
		}
		for (int l16 = 0; (l16 < 2048); l16 = (l16 + 1)) {
			fVec3[l16] = 0.0f;
		}
		for (int l17 = 0; (l17 < 2); l17 = (l17 + 1)) {
			fRec17[l17] = 0.0f;
		}
		for (int l18 = 0; (l18 < 131072); l18 = (l18 + 1)) {
			fVec4[l18] = 0.0f;
		}
		for (int l19 = 0; (l19 < 2); l19 = (l19 + 1)) {
			fRec9[l19] = 0.0f;
		}
		for (int l20 = 0; (l20 < 32768); l20 = (l20 + 1)) {
			fVec5[l20] = 0.0f;
		}
		for (int l21 = 0; (l21 < 2); l21 = (l21 + 1)) {
			fRec29[l21] = 0.0f;
		}
		for (int l22 = 0; (l22 < 2); l22 = (l22 + 1)) {
			fRec8[l22] = 0.0f;
		}
		for (int l23 = 0; (l23 < 32768); l23 = (l23 + 1)) {
			fVec6[l23] = 0.0f;
		}
		for (int l24 = 0; (l24 < 2); l24 = (l24 + 1)) {
			fRec6[l24] = 0.0f;
		}
		for (int l25 = 0; (l25 < 32768); l25 = (l25 + 1)) {
			fRec0[l25] = 0.0f;
		}
		for (int l26 = 0; (l26 < 16384); l26 = (l26 + 1)) {
			fRec1[l26] = 0.0f;
		}
		for (int l27 = 0; (l27 < 32768); l27 = (l27 + 1)) {
			fRec2[l27] = 0.0f;
		}
		for (int l28 = 0; (l28 < 2); l28 = (l28 + 1)) {
			fRec35[l28] = 0.0f;
		}
		for (int l29 = 0; (l29 < 2); l29 = (l29 + 1)) {
			fRec36[l29] = 0.0f;
		}
		for (int l30 = 0; (l30 < 2); l30 = (l30 + 1)) {
			iRec37[l30] = 0;
		}
		for (int l31 = 0; (l31 < 2); l31 = (l31 + 1)) {
			iRec38[l31] = 0;
		}
		for (int l32 = 0; (l32 < 2); l32 = (l32 + 1)) {
			fRec48[l32] = 0.0f;
		}
		for (int l33 = 0; (l33 < 2); l33 = (l33 + 1)) {
			fRec47[l33] = 0.0f;
		}
		for (int l34 = 0; (l34 < 1024); l34 = (l34 + 1)) {
			fVec7[l34] = 0.0f;
		}
		for (int l35 = 0; (l35 < 2); l35 = (l35 + 1)) {
			fRec45[l35] = 0.0f;
		}
		for (int l36 = 0; (l36 < 1024); l36 = (l36 + 1)) {
			fVec8[l36] = 0.0f;
		}
		for (int l37 = 0; (l37 < 2); l37 = (l37 + 1)) {
			fRec43[l37] = 0.0f;
		}
		for (int l38 = 0; (l38 < 4096); l38 = (l38 + 1)) {
			fVec9[l38] = 0.0f;
		}
		for (int l39 = 0; (l39 < 2); l39 = (l39 + 1)) {
			fRec41[l39] = 0.0f;
		}
		for (int l40 = 0; (l40 < 2048); l40 = (l40 + 1)) {
			fVec10[l40] = 0.0f;
		}
		for (int l41 = 0; (l41 < 2); l41 = (l41 + 1)) {
			fRec39[l41] = 0.0f;
		}
		for (int l42 = 0; (l42 < 131072); l42 = (l42 + 1)) {
			fVec11[l42] = 0.0f;
		}
		for (int l43 = 0; (l43 < 2); l43 = (l43 + 1)) {
			fRec33[l43] = 0.0f;
		}
		for (int l44 = 0; (l44 < 32768); l44 = (l44 + 1)) {
			fVec12[l44] = 0.0f;
		}
		for (int l45 = 0; (l45 < 2); l45 = (l45 + 1)) {
			fRec32[l45] = 0.0f;
		}
		for (int l46 = 0; (l46 < 16384); l46 = (l46 + 1)) {
			fVec13[l46] = 0.0f;
		}
		for (int l47 = 0; (l47 < 2); l47 = (l47 + 1)) {
			fRec30[l47] = 0.0f;
		}
		for (int l48 = 0; (l48 < 32768); l48 = (l48 + 1)) {
			fRec3[l48] = 0.0f;
		}
		for (int l49 = 0; (l49 < 8192); l49 = (l49 + 1)) {
			fRec4[l49] = 0.0f;
		}
		for (int l50 = 0; (l50 < 32768); l50 = (l50 + 1)) {
			fRec5[l50] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual Reverb_dsp* clone() {
		return new Reverb_dsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fverb");
		ui_interface->declare(&fHslider1, "0", "");
		ui_interface->declare(&fHslider1, "symbol", "decay");
		ui_interface->declare(&fHslider1, "unit", "%");
		ui_interface->addHorizontalSlider("Decay", &fHslider1, 50.0f, 0.0f, 100.0f, 0.00999999978f);
		ui_interface->declare(&fHslider0, "1", "");
		ui_interface->declare(&fHslider0, "symbol", "wet");
		ui_interface->declare(&fHslider0, "unit", "%");
		ui_interface->addHorizontalSlider("Wet", &fHslider0, 50.0f, 0.0f, 100.0f, 0.00999999978f);
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = (0.00600000005f * float(fHslider0));
		float fSlow1 = (0.00999999978f * float(fHslider1));
		float fSlow2 = std::min<float>(0.5f, std::max<float>(0.25f, (fSlow1 + 0.150000006f)));
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			float fTemp1 = float(input1[i]);
			fRec16[0] = (fConst1 + (fRec16[1] - float(int((fConst1 + fRec16[1])))));
			int iTemp2 = (int((fConst0 * ((0.000500000024f * ftbl0Reverb_dspSIG0[int((65536.0f * (fRec16[0] + (0.25f - float(int((fRec16[0] + 0.25f)))))))]) + 0.0305097271f))) + -1);
			float fTemp3 = ((fRec11[1] != 0.0f) ? (((fRec12[1] > 0.0f) & (fRec12[1] < 1.0f)) ? fRec11[1] : 0.0f) : (((fRec12[1] == 0.0f) & (iTemp2 != iRec13[1])) ? fConst2 : (((fRec12[1] == 1.0f) & (iTemp2 != iRec14[1])) ? fConst3 : 0.0f)));
			fRec11[0] = fTemp3;
			fRec12[0] = std::max<float>(0.0f, std::min<float>(1.0f, (fRec12[1] + fTemp3)));
			iRec13[0] = (((fRec12[1] >= 1.0f) & (iRec14[1] != iTemp2)) ? iTemp2 : iRec13[1]);
			iRec14[0] = (((fRec12[1] <= 0.0f) & (iRec13[1] != iTemp2)) ? iTemp2 : iRec14[1]);
			fRec27[0] = (fConst4 + (0.999000013f * fRec27[1]));
			fRec26[0] = (fTemp1 + (fRec27[0] * fRec26[1]));
			float fTemp4 = (1.0f - fRec27[0]);
			fRec28[0] = (fConst5 + (0.999000013f * fRec28[1]));
			fRec25[0] = ((fRec26[0] * fTemp4) + (fRec28[0] * fRec25[1]));
			float fTemp5 = (fRec28[0] + 1.0f);
			float fTemp6 = (0.0f - (0.5f * fTemp5));
			float fTemp7 = (((0.5f * (fRec25[0] * fTemp5)) + (fRec25[1] * fTemp6)) - (0.75f * fRec23[1]));
			fVec0[(IOTA & 1023)] = fTemp7;
			fRec23[0] = fVec0[((IOTA - iConst6) & 1023)];
			float fRec24 = (0.75f * fTemp7);
			float fTemp8 = ((fRec24 + fRec23[1]) - (0.75f * fRec21[1]));
			fVec1[(IOTA & 1023)] = fTemp8;
			fRec21[0] = fVec1[((IOTA - iConst7) & 1023)];
			float fRec22 = (0.75f * fTemp8);
			float fTemp9 = ((fRec22 + fRec21[1]) - (0.625f * fRec19[1]));
			fVec2[(IOTA & 4095)] = fTemp9;
			fRec19[0] = fVec2[((IOTA - iConst8) & 4095)];
			float fRec20 = (0.625f * fTemp9);
			float fTemp10 = ((fRec20 + fRec19[1]) - (0.625f * fRec17[1]));
			fVec3[(IOTA & 2047)] = fTemp10;
			fRec17[0] = fVec3[((IOTA - iConst9) & 2047)];
			float fRec18 = (0.625f * fTemp10);
			float fTemp11 = (fRec17[1] + ((fSlow1 * fRec3[((IOTA - iConst10) & 32767)]) + (fRec18 + (0.0700000003f * fRec9[1]))));
			fVec4[(IOTA & 131071)] = fTemp11;
			fRec9[0] = (((1.0f - fRec12[0]) * fVec4[((IOTA - std::min<int>(65536, std::max<int>(0, iRec13[0]))) & 131071)]) + (fRec12[0] * fVec4[((IOTA - std::min<int>(65536, std::max<int>(0, iRec14[0]))) & 131071)]));
			float fRec10 = (0.0f - (0.0700000003f * fTemp11));
			float fTemp12 = (fRec10 + fRec9[1]);
			fVec5[(IOTA & 32767)] = fTemp12;
			fRec29[0] = (fConst12 + (0.999000013f * fRec29[1]));
			fRec8[0] = (fVec5[((IOTA - iConst11) & 32767)] + (fRec29[0] * fRec8[1]));
			float fTemp13 = (1.0f - fRec29[0]);
			float fTemp14 = (fRec8[0] * fTemp13);
			float fTemp15 = ((fSlow2 * fRec6[1]) + (fSlow1 * fTemp14));
			fVec6[(IOTA & 32767)] = fTemp15;
			fRec6[0] = fVec6[((IOTA - iConst13) & 32767)];
			float fRec7 = (0.0f - (fSlow2 * fTemp15));
			fRec0[(IOTA & 32767)] = (fRec7 + fRec6[1]);
			fRec1[(IOTA & 16383)] = fTemp14;
			fRec2[(IOTA & 32767)] = fTemp12;
			int iTemp16 = (int((fConst0 * ((0.000500000024f * ftbl0Reverb_dspSIG0[int((65536.0f * fRec16[0]))]) + 0.025603978f))) + -1);
			float fTemp17 = ((fRec35[1] != 0.0f) ? (((fRec36[1] > 0.0f) & (fRec36[1] < 1.0f)) ? fRec35[1] : 0.0f) : (((fRec36[1] == 0.0f) & (iTemp16 != iRec37[1])) ? fConst2 : (((fRec36[1] == 1.0f) & (iTemp16 != iRec38[1])) ? fConst3 : 0.0f)));
			fRec35[0] = fTemp17;
			fRec36[0] = std::max<float>(0.0f, std::min<float>(1.0f, (fRec36[1] + fTemp17)));
			iRec37[0] = (((fRec36[1] >= 1.0f) & (iRec38[1] != iTemp16)) ? iTemp16 : iRec37[1]);
			iRec38[0] = (((fRec36[1] <= 0.0f) & (iRec37[1] != iTemp16)) ? iTemp16 : iRec38[1]);
			fRec48[0] = (fTemp0 + (fRec27[0] * fRec48[1]));
			fRec47[0] = ((fTemp4 * fRec48[0]) + (fRec28[0] * fRec47[1]));
			float fTemp18 = (((0.5f * (fRec47[0] * fTemp5)) + (fTemp6 * fRec47[1])) - (0.75f * fRec45[1]));
			fVec7[(IOTA & 1023)] = fTemp18;
			fRec45[0] = fVec7[((IOTA - iConst14) & 1023)];
			float fRec46 = (0.75f * fTemp18);
			float fTemp19 = ((fRec46 + fRec45[1]) - (0.75f * fRec43[1]));
			fVec8[(IOTA & 1023)] = fTemp19;
			fRec43[0] = fVec8[((IOTA - iConst15) & 1023)];
			float fRec44 = (0.75f * fTemp19);
			float fTemp20 = ((fRec44 + fRec43[1]) - (0.625f * fRec41[1]));
			fVec9[(IOTA & 4095)] = fTemp20;
			fRec41[0] = fVec9[((IOTA - iConst16) & 4095)];
			float fRec42 = (0.625f * fTemp20);
			float fTemp21 = ((fRec42 + fRec41[1]) - (0.625f * fRec39[1]));
			fVec10[(IOTA & 2047)] = fTemp21;
			fRec39[0] = fVec10[((IOTA - iConst17) & 2047)];
			float fRec40 = (0.625f * fTemp21);
			float fTemp22 = (fRec39[1] + ((fSlow1 * fRec0[((IOTA - iConst18) & 32767)]) + (fRec40 + (0.0700000003f * fRec33[1]))));
			fVec11[(IOTA & 131071)] = fTemp22;
			fRec33[0] = (((1.0f - fRec36[0]) * fVec11[((IOTA - std::min<int>(65536, std::max<int>(0, iRec37[0]))) & 131071)]) + (fRec36[0] * fVec11[((IOTA - std::min<int>(65536, std::max<int>(0, iRec38[0]))) & 131071)]));
			float fRec34 = (0.0f - (0.0700000003f * fTemp22));
			float fTemp23 = (fRec34 + fRec33[1]);
			fVec12[(IOTA & 32767)] = fTemp23;
			fRec32[0] = (fVec12[((IOTA - iConst19) & 32767)] + (fRec29[0] * fRec32[1]));
			float fTemp24 = (fTemp13 * fRec32[0]);
			float fTemp25 = ((fSlow2 * fRec30[1]) + (fSlow1 * fTemp24));
			fVec13[(IOTA & 16383)] = fTemp25;
			fRec30[0] = fVec13[((IOTA - iConst20) & 16383)];
			float fRec31 = (0.0f - (fSlow2 * fTemp25));
			fRec3[(IOTA & 32767)] = (fRec31 + fRec30[1]);
			fRec4[(IOTA & 8191)] = fTemp24;
			fRec5[(IOTA & 32767)] = fTemp23;
			output0[i] = FAUSTFLOAT((fTemp0 + (fSlow0 * (((fRec2[((IOTA - iConst21) & 32767)] + fRec2[((IOTA - iConst22) & 32767)]) + fRec0[((IOTA - iConst23) & 32767)]) - (((fRec1[((IOTA - iConst24) & 16383)] + fRec5[((IOTA - iConst25) & 32767)]) + fRec4[((IOTA - iConst26) & 8191)]) + fRec3[((IOTA - iConst27) & 32767)])))));
			output1[i] = FAUSTFLOAT((fTemp1 + (fSlow0 * (((fRec5[((IOTA - iConst28) & 32767)] + fRec5[((IOTA - iConst29) & 32767)]) + fRec3[((IOTA - iConst30) & 32767)]) - (((fRec4[((IOTA - iConst31) & 8191)] + fRec2[((IOTA - iConst32) & 32767)]) + fRec1[((IOTA - iConst33) & 16383)]) + fRec0[((IOTA - iConst34) & 32767)])))));
			fRec16[1] = fRec16[0];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
			iRec13[1] = iRec13[0];
			iRec14[1] = iRec14[0];
			fRec27[1] = fRec27[0];
			fRec26[1] = fRec26[0];
			fRec28[1] = fRec28[0];
			fRec25[1] = fRec25[0];
			IOTA = (IOTA + 1);
			fRec23[1] = fRec23[0];
			fRec21[1] = fRec21[0];
			fRec19[1] = fRec19[0];
			fRec17[1] = fRec17[0];
			fRec9[1] = fRec9[0];
			fRec29[1] = fRec29[0];
			fRec8[1] = fRec8[0];
			fRec6[1] = fRec6[0];
			fRec35[1] = fRec35[0];
			fRec36[1] = fRec36[0];
			iRec37[1] = iRec37[0];
			iRec38[1] = iRec38[0];
			fRec48[1] = fRec48[0];
			fRec47[1] = fRec47[0];
			fRec45[1] = fRec45[0];
			fRec43[1] = fRec43[0];
			fRec41[1] = fRec41[0];
			fRec39[1] = fRec39[0];
			fRec33[1] = fRec33[0];
			fRec32[1] = fRec32[0];
			fRec30[1] = fRec30[0];
		}
	}

};

#endif
