/* ------------------------------------------------------------
name: "eq_5band"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __Eq_5band_dsp_H__
#define  __Eq_5band_dsp_H__

#include "audio/wrap/faust.h"


#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>

static float Eq_5band_dsp_faustpower2_f(float value) {
	return (value * value);
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS Eq_5band_dsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class Eq_5band_dsp : public dsp {
	
 private:
	
	float fVec0[2];
	float fVec1[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	float fConst3;
	float fConst4;
	float fConst5;
	float fConst6;
	float fConst7;
	float fConst8;
	float fConst9;
	float fConst10;
	float fConst11;
	float fConst12;
	float fConst13;
	float fConst14;
	float fConst15;
	float fConst16;
	float fConst17;
	float fConst18;
	float fConst19;
	float fConst20;
	float fConst21;
	float fRec6[2];
	float fConst22;
	float fConst23;
	float fRec5[3];
	float fConst24;
	FAUSTFLOAT fHslider0;
	float fRec8[2];
	float fRec7[3];
	float fConst25;
	FAUSTFLOAT fHslider1;
	float fConst26;
	float fRec4[3];
	float fConst27;
	FAUSTFLOAT fHslider2;
	float fConst28;
	float fRec3[3];
	float fConst29;
	FAUSTFLOAT fHslider3;
	float fConst30;
	float fRec2[3];
	float fVec2[2];
	float fRec1[2];
	float fConst31;
	float fConst32;
	float fConst33;
	float fConst34;
	float fRec0[3];
	FAUSTFLOAT fHslider4;
	float fConst35;
	float fRec10[2];
	float fRec9[3];
	float fConst36;
	float fRec17[2];
	float fRec16[3];
	float fRec19[2];
	float fRec18[3];
	float fRec15[3];
	float fRec14[3];
	float fRec13[3];
	float fVec3[2];
	float fRec12[2];
	float fRec11[3];
	float fRec21[2];
	float fRec20[3];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "0.0");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.1");
		m->declare("filename", "eq_5band.dsp");
		m->declare("filters.lib/filterbank:author", "Julius O. Smith III");
		m->declare("filters.lib/filterbank:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/filterbank:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/highshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highshelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/low_shelf:author", "Julius O. Smith III");
		m->declare("filters.lib/low_shelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/low_shelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/lowshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowshelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/peak_eq:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.1");
		m->declare("name", "eq_5band");
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
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = std::tan((13150.7354f / fConst0));
		fConst2 = (1.0f / fConst1);
		fConst3 = (1.0f / (((fConst2 + 1.0f) / fConst1) + 1.0f));
		fConst4 = (fConst2 + 1.0f);
		fConst5 = (1.0f / fConst4);
		fConst6 = (1.0f - fConst2);
		fConst7 = std::tan((5529.20312f / fConst0));
		fConst8 = (2.0f * (1.0f - (1.0f / Eq_5band_dsp_faustpower2_f(fConst7))));
		fConst9 = std::tan((2324.74341f / fConst0));
		fConst10 = (2.0f * (1.0f - (1.0f / Eq_5band_dsp_faustpower2_f(fConst9))));
		fConst11 = std::tan((977.434265f / fConst0));
		fConst12 = (2.0f * (1.0f - (1.0f / Eq_5band_dsp_faustpower2_f(fConst11))));
		fConst13 = std::tan((410.96048f / fConst0));
		fConst14 = (1.0f / fConst13);
		fConst15 = (1.0f / (((fConst14 + 1.0f) / fConst13) + 1.0f));
		fConst16 = Eq_5band_dsp_faustpower2_f(fConst13);
		fConst17 = (1.0f / fConst16);
		fConst18 = (fConst14 + 1.0f);
		fConst19 = (0.0f - (1.0f / (fConst13 * fConst18)));
		fConst20 = (1.0f / fConst18);
		fConst21 = (1.0f - fConst14);
		fConst22 = (((fConst14 + -1.0f) / fConst13) + 1.0f);
		fConst23 = (2.0f * (1.0f - fConst17));
		fConst24 = (0.0f - (2.0f / fConst16));
		fConst25 = (1.0f / fConst11);
		fConst26 = (673.654663f / (fConst0 * std::sin((1954.86853f / fConst0))));
		fConst27 = (1.0f / fConst9);
		fConst28 = (1602.22974f / (fConst0 * std::sin((4649.48682f / fConst0))));
		fConst29 = (1.0f / fConst7);
		fConst30 = (3810.76611f / (fConst0 * std::sin((11058.4062f / fConst0))));
		fConst31 = (((fConst2 + -1.0f) / fConst1) + 1.0f);
		fConst32 = Eq_5band_dsp_faustpower2_f(fConst1);
		fConst33 = (1.0f / fConst32);
		fConst34 = (2.0f * (1.0f - fConst33));
		fConst35 = (0.0f - (1.0f / (fConst1 * fConst4)));
		fConst36 = (0.0f - (2.0f / fConst32));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(50.0f);
		fHslider1 = FAUSTFLOAT(50.0f);
		fHslider2 = FAUSTFLOAT(50.0f);
		fHslider3 = FAUSTFLOAT(50.0f);
		fHslider4 = FAUSTFLOAT(50.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fVec0[l0] = 0.0f;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fVec1[l1] = 0.0f;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec6[l2] = 0.0f;
		}
		for (int l3 = 0; (l3 < 3); l3 = (l3 + 1)) {
			fRec5[l3] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec8[l4] = 0.0f;
		}
		for (int l5 = 0; (l5 < 3); l5 = (l5 + 1)) {
			fRec7[l5] = 0.0f;
		}
		for (int l6 = 0; (l6 < 3); l6 = (l6 + 1)) {
			fRec4[l6] = 0.0f;
		}
		for (int l7 = 0; (l7 < 3); l7 = (l7 + 1)) {
			fRec3[l7] = 0.0f;
		}
		for (int l8 = 0; (l8 < 3); l8 = (l8 + 1)) {
			fRec2[l8] = 0.0f;
		}
		for (int l9 = 0; (l9 < 2); l9 = (l9 + 1)) {
			fVec2[l9] = 0.0f;
		}
		for (int l10 = 0; (l10 < 2); l10 = (l10 + 1)) {
			fRec1[l10] = 0.0f;
		}
		for (int l11 = 0; (l11 < 3); l11 = (l11 + 1)) {
			fRec0[l11] = 0.0f;
		}
		for (int l12 = 0; (l12 < 2); l12 = (l12 + 1)) {
			fRec10[l12] = 0.0f;
		}
		for (int l13 = 0; (l13 < 3); l13 = (l13 + 1)) {
			fRec9[l13] = 0.0f;
		}
		for (int l14 = 0; (l14 < 2); l14 = (l14 + 1)) {
			fRec17[l14] = 0.0f;
		}
		for (int l15 = 0; (l15 < 3); l15 = (l15 + 1)) {
			fRec16[l15] = 0.0f;
		}
		for (int l16 = 0; (l16 < 2); l16 = (l16 + 1)) {
			fRec19[l16] = 0.0f;
		}
		for (int l17 = 0; (l17 < 3); l17 = (l17 + 1)) {
			fRec18[l17] = 0.0f;
		}
		for (int l18 = 0; (l18 < 3); l18 = (l18 + 1)) {
			fRec15[l18] = 0.0f;
		}
		for (int l19 = 0; (l19 < 3); l19 = (l19 + 1)) {
			fRec14[l19] = 0.0f;
		}
		for (int l20 = 0; (l20 < 3); l20 = (l20 + 1)) {
			fRec13[l20] = 0.0f;
		}
		for (int l21 = 0; (l21 < 2); l21 = (l21 + 1)) {
			fVec3[l21] = 0.0f;
		}
		for (int l22 = 0; (l22 < 2); l22 = (l22 + 1)) {
			fRec12[l22] = 0.0f;
		}
		for (int l23 = 0; (l23 < 3); l23 = (l23 + 1)) {
			fRec11[l23] = 0.0f;
		}
		for (int l24 = 0; (l24 < 2); l24 = (l24 + 1)) {
			fRec21[l24] = 0.0f;
		}
		for (int l25 = 0; (l25 < 3); l25 = (l25 + 1)) {
			fRec20[l25] = 0.0f;
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
	
	virtual Eq_5band_dsp* clone() {
		return new Eq_5band_dsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("eq_5band");
		ui_interface->declare(&fHslider0, "0", "");
		ui_interface->declare(&fHslider0, "unit", "%");
		ui_interface->addHorizontalSlider("EQ Band 1", &fHslider0, 50.0f, 0.0f, 100.0f, 1.0f);
		ui_interface->declare(&fHslider1, "1", "");
		ui_interface->declare(&fHslider1, "unit", "%");
		ui_interface->addHorizontalSlider("EQ Band 2", &fHslider1, 50.0f, 0.0f, 100.0f, 1.0f);
		ui_interface->declare(&fHslider2, "2", "");
		ui_interface->declare(&fHslider2, "unit", "%");
		ui_interface->addHorizontalSlider("EQ Band 3", &fHslider2, 50.0f, 0.0f, 100.0f, 1.0f);
		ui_interface->declare(&fHslider3, "3", "");
		ui_interface->declare(&fHslider3, "unit", "%");
		ui_interface->addHorizontalSlider("EQ Band 4", &fHslider3, 50.0f, 0.0f, 100.0f, 1.0f);
		ui_interface->declare(&fHslider4, "4", "");
		ui_interface->declare(&fHslider4, "unit", "%");
		ui_interface->addHorizontalSlider("EQ Band 5", &fHslider4, 50.0f, 0.0f, 100.0f, 1.0f);
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = std::pow(10.0f, (0.0500000007f * ((0.239999995f * float(fHslider0)) + -12.0f)));
		float fSlow1 = ((0.239999995f * float(fHslider1)) + -12.0f);
		int iSlow2 = (fSlow1 > 0.0f);
		float fSlow3 = (fConst26 * std::pow(10.0f, (0.0500000007f * std::fabs(fSlow1))));
		float fSlow4 = (iSlow2 ? fConst26 : fSlow3);
		float fSlow5 = (1.0f - (fConst25 * (fSlow4 - fConst25)));
		float fSlow6 = ((fConst25 * (fConst25 + fSlow4)) + 1.0f);
		float fSlow7 = (iSlow2 ? fSlow3 : fConst26);
		float fSlow8 = ((fConst25 * (fConst25 + fSlow7)) + 1.0f);
		float fSlow9 = (1.0f - (fConst25 * (fSlow7 - fConst25)));
		float fSlow10 = ((0.239999995f * float(fHslider2)) + -12.0f);
		int iSlow11 = (fSlow10 > 0.0f);
		float fSlow12 = (fConst28 * std::pow(10.0f, (0.0500000007f * std::fabs(fSlow10))));
		float fSlow13 = (iSlow11 ? fConst28 : fSlow12);
		float fSlow14 = (1.0f - (fConst27 * (fSlow13 - fConst27)));
		float fSlow15 = ((fConst27 * (fConst27 + fSlow13)) + 1.0f);
		float fSlow16 = (iSlow11 ? fSlow12 : fConst28);
		float fSlow17 = ((fConst27 * (fConst27 + fSlow16)) + 1.0f);
		float fSlow18 = (1.0f - (fConst27 * (fSlow16 - fConst27)));
		float fSlow19 = ((0.239999995f * float(fHslider3)) + -12.0f);
		int iSlow20 = (fSlow19 > 0.0f);
		float fSlow21 = (fConst30 * std::pow(10.0f, (0.0500000007f * std::fabs(fSlow19))));
		float fSlow22 = (iSlow20 ? fConst30 : fSlow21);
		float fSlow23 = (1.0f - (fConst29 * (fSlow22 - fConst29)));
		float fSlow24 = ((fConst29 * (fConst29 + fSlow22)) + 1.0f);
		float fSlow25 = (iSlow20 ? fSlow21 : fConst30);
		float fSlow26 = ((fConst29 * (fConst29 + fSlow25)) + 1.0f);
		float fSlow27 = (1.0f - (fConst29 * (fSlow25 - fConst29)));
		float fSlow28 = std::pow(10.0f, (0.0500000007f * ((0.239999995f * float(fHslider4)) + -12.0f)));
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			fVec0[0] = fTemp0;
			float fTemp1 = float(input1[i]);
			fVec1[0] = fTemp1;
			fRec6[0] = ((fConst19 * fVec0[1]) - (fConst20 * ((fConst21 * fRec6[1]) - (fConst14 * fTemp0))));
			fRec5[0] = (fRec6[0] - (fConst15 * ((fConst22 * fRec5[2]) + (fConst23 * fRec5[1]))));
			fRec8[0] = (0.0f - (fConst20 * ((fConst21 * fRec8[1]) - (fTemp0 + fVec0[1]))));
			fRec7[0] = (fRec8[0] - (fConst15 * ((fConst22 * fRec7[2]) + (fConst23 * fRec7[1]))));
			float fTemp2 = (fConst12 * fRec4[1]);
			fRec4[0] = ((fConst15 * ((((fConst17 * fRec5[0]) + (fConst24 * fRec5[1])) + (fConst17 * fRec5[2])) + (fSlow0 * (fRec7[2] + (fRec7[0] + (2.0f * fRec7[1])))))) - (((fRec4[2] * fSlow5) + fTemp2) / fSlow6));
			float fTemp3 = (fConst10 * fRec3[1]);
			fRec3[0] = ((((fTemp2 + (fRec4[0] * fSlow8)) + (fRec4[2] * fSlow9)) / fSlow6) - (((fRec3[2] * fSlow14) + fTemp3) / fSlow15));
			float fTemp4 = (fConst8 * fRec2[1]);
			fRec2[0] = ((((fTemp3 + (fRec3[0] * fSlow17)) + (fRec3[2] * fSlow18)) / fSlow15) - (((fRec2[2] * fSlow23) + fTemp4) / fSlow24));
			float fTemp5 = (((fTemp4 + (fRec2[0] * fSlow26)) + (fRec2[2] * fSlow27)) / fSlow24);
			fVec2[0] = fTemp5;
			fRec1[0] = (0.0f - (fConst5 * ((fConst6 * fRec1[1]) - (fTemp5 + fVec2[1]))));
			fRec0[0] = (fRec1[0] - (fConst3 * ((fConst31 * fRec0[2]) + (fConst34 * fRec0[1]))));
			fRec10[0] = ((fConst35 * fVec2[1]) - (fConst5 * ((fConst6 * fRec10[1]) - (fConst2 * fTemp5))));
			fRec9[0] = (fRec10[0] - (fConst3 * ((fConst31 * fRec9[2]) + (fConst34 * fRec9[1]))));
			output0[i] = FAUSTFLOAT((fConst3 * ((fRec0[2] + (fRec0[0] + (2.0f * fRec0[1]))) + (fSlow28 * (((fConst33 * fRec9[0]) + (fConst36 * fRec9[1])) + (fConst33 * fRec9[2]))))));
			fRec17[0] = ((fConst19 * fVec1[1]) - (fConst20 * ((fConst21 * fRec17[1]) - (fConst14 * fTemp1))));
			fRec16[0] = (fRec17[0] - (fConst15 * ((fConst22 * fRec16[2]) + (fConst23 * fRec16[1]))));
			fRec19[0] = (0.0f - (fConst20 * ((fConst21 * fRec19[1]) - (fTemp1 + fVec1[1]))));
			fRec18[0] = (fRec19[0] - (fConst15 * ((fConst22 * fRec18[2]) + (fConst23 * fRec18[1]))));
			float fTemp6 = (fConst12 * fRec15[1]);
			fRec15[0] = ((fConst15 * ((((fConst17 * fRec16[0]) + (fConst24 * fRec16[1])) + (fConst17 * fRec16[2])) + (fSlow0 * (fRec18[2] + (fRec18[0] + (2.0f * fRec18[1])))))) - (((fSlow5 * fRec15[2]) + fTemp6) / fSlow6));
			float fTemp7 = (fConst10 * fRec14[1]);
			fRec14[0] = ((((fTemp6 + (fRec15[0] * fSlow8)) + (fSlow9 * fRec15[2])) / fSlow6) - (((fSlow14 * fRec14[2]) + fTemp7) / fSlow15));
			float fTemp8 = (fConst8 * fRec13[1]);
			fRec13[0] = ((((fTemp7 + (fRec14[0] * fSlow17)) + (fSlow18 * fRec14[2])) / fSlow15) - (((fSlow23 * fRec13[2]) + fTemp8) / fSlow24));
			float fTemp9 = (((fTemp8 + (fRec13[0] * fSlow26)) + (fSlow27 * fRec13[2])) / fSlow24);
			fVec3[0] = fTemp9;
			fRec12[0] = (0.0f - (fConst5 * ((fConst6 * fRec12[1]) - (fTemp9 + fVec3[1]))));
			fRec11[0] = (fRec12[0] - (fConst3 * ((fConst31 * fRec11[2]) + (fConst34 * fRec11[1]))));
			fRec21[0] = ((fConst35 * fVec3[1]) - (fConst5 * ((fConst6 * fRec21[1]) - (fConst2 * fTemp9))));
			fRec20[0] = (fRec21[0] - (fConst3 * ((fConst31 * fRec20[2]) + (fConst34 * fRec20[1]))));
			output1[i] = FAUSTFLOAT((fConst3 * ((fRec11[2] + (fRec11[0] + (2.0f * fRec11[1]))) + (fSlow28 * (((fConst33 * fRec20[0]) + (fConst36 * fRec20[1])) + (fConst33 * fRec20[2]))))));
			fVec0[1] = fVec0[0];
			fVec1[1] = fVec1[0];
			fRec6[1] = fRec6[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec8[1] = fRec8[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fVec2[1] = fVec2[0];
			fRec1[1] = fRec1[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec10[1] = fRec10[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec17[1] = fRec17[0];
			fRec16[2] = fRec16[1];
			fRec16[1] = fRec16[0];
			fRec19[1] = fRec19[0];
			fRec18[2] = fRec18[1];
			fRec18[1] = fRec18[0];
			fRec15[2] = fRec15[1];
			fRec15[1] = fRec15[0];
			fRec14[2] = fRec14[1];
			fRec14[1] = fRec14[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fVec3[1] = fVec3[0];
			fRec12[1] = fRec12[0];
			fRec11[2] = fRec11[1];
			fRec11[1] = fRec11[0];
			fRec21[1] = fRec21[0];
			fRec20[2] = fRec20[1];
			fRec20[1] = fRec20[0];
		}
	}

};

#endif
