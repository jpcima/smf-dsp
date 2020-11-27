/* ------------------------------------------------------------
name: "bass_enhance"
Code generated with Faust 2.27.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __BassEnhance_dsp_H__
#define  __BassEnhance_dsp_H__

#include "audio/wrap/faust.h"


#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>

static float BassEnhance_dsp_faustpower2_f(float value) {
	return (value * value);
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS BassEnhance_dsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class BassEnhance_dsp : public dsp {
	
 private:
	
	float fVec0[2];
	float fVec1[2];
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	FAUSTFLOAT fHslider1;
	float fRec1[2];
	float fRec0[3];
	float fRec3[2];
	float fRec2[3];
	float fRec5[2];
	float fRec4[3];
	float fRec7[2];
	float fRec6[3];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "0.1");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.1");
		m->declare("filename", "bass_enhance.dsp");
		m->declare("filters.lib/filterbank:author", "Julius O. Smith III");
		m->declare("filters.lib/filterbank:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/filterbank:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
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
		m->declare("maths.lib/version", "2.3");
		m->declare("misceffects.lib/name", "Misc Effects Library");
		m->declare("misceffects.lib/version", "2.0");
		m->declare("name", "bass_enhance");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "0.1");
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
		fConst0 = (3.14159274f / std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate))));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.0f);
		fHslider1 = FAUSTFLOAT(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fVec0[l0] = 0.0f;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fVec1[l1] = 0.0f;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec1[l2] = 0.0f;
		}
		for (int l3 = 0; (l3 < 3); l3 = (l3 + 1)) {
			fRec0[l3] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec3[l4] = 0.0f;
		}
		for (int l5 = 0; (l5 < 3); l5 = (l5 + 1)) {
			fRec2[l5] = 0.0f;
		}
		for (int l6 = 0; (l6 < 2); l6 = (l6 + 1)) {
			fRec5[l6] = 0.0f;
		}
		for (int l7 = 0; (l7 < 3); l7 = (l7 + 1)) {
			fRec4[l7] = 0.0f;
		}
		for (int l8 = 0; (l8 < 2); l8 = (l8 + 1)) {
			fRec7[l8] = 0.0f;
		}
		for (int l9 = 0; (l9 < 3); l9 = (l9 + 1)) {
			fRec6[l9] = 0.0f;
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
	
	virtual BassEnhance_dsp* clone() {
		return new BassEnhance_dsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("bass_enhance");
		ui_interface->declare(&fHslider0, "0", "");
		ui_interface->declare(&fHslider0, "unit", "%");
		ui_interface->addHorizontalSlider("Drive", &fHslider0, 0.0f, 0.0f, 100.0f, 0.00999999978f);
		ui_interface->declare(&fHslider1, "1", "");
		ui_interface->declare(&fHslider1, "unit", "%");
		ui_interface->addHorizontalSlider("Tone", &fHslider1, 0.0f, 0.0f, 100.0f, 0.00999999978f);
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = std::tan((fConst0 * ((5.0f * float(fHslider1)) + 100.0f)));
		float fSlow1 = (1.0f / fSlow0);
		float fSlow2 = (((fSlow1 + 1.0f) / fSlow0) + 1.0f);
		float fSlow3 = (std::pow(10.0f, (0.00999999978f * float(fHslider0))) / fSlow2);
		float fSlow4 = (fSlow1 + 1.0f);
		float fSlow5 = (1.0f / fSlow4);
		float fSlow6 = (1.0f - fSlow1);
		float fSlow7 = (1.0f / fSlow2);
		float fSlow8 = (((fSlow1 + -1.0f) / fSlow0) + 1.0f);
		float fSlow9 = BassEnhance_dsp_faustpower2_f(fSlow0);
		float fSlow10 = (1.0f / fSlow9);
		float fSlow11 = (2.0f * (1.0f - fSlow10));
		float fSlow12 = (0.0f - (2.0f / fSlow9));
		float fSlow13 = (0.0f - (1.0f / (fSlow0 * fSlow4)));
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			fVec0[0] = fTemp0;
			float fTemp1 = float(input1[i]);
			fVec1[0] = fTemp1;
			fRec1[0] = (0.0f - (fSlow5 * ((fSlow6 * fRec1[1]) - (fTemp0 + fVec0[1]))));
			fRec0[0] = (fRec1[0] - (fSlow7 * ((fSlow8 * fRec0[2]) + (fSlow11 * fRec0[1]))));
			float fTemp2 = std::max<float>(-1.0f, std::min<float>(1.0f, (fSlow3 * (fRec0[2] + (fRec0[0] + (2.0f * fRec0[1]))))));
			fRec3[0] = ((fSlow13 * fVec0[1]) - (fSlow5 * ((fSlow6 * fRec3[1]) - (fSlow1 * fTemp0))));
			fRec2[0] = (fRec3[0] - (fSlow7 * ((fSlow8 * fRec2[2]) + (fSlow11 * fRec2[1]))));
			output0[i] = FAUSTFLOAT(((fTemp2 * (1.0f - (0.333333343f * BassEnhance_dsp_faustpower2_f(fTemp2)))) + (fSlow7 * (((fSlow12 * fRec2[1]) + (fSlow10 * fRec2[0])) + (fSlow10 * fRec2[2])))));
			fRec5[0] = (0.0f - (fSlow5 * ((fSlow6 * fRec5[1]) - (fTemp1 + fVec1[1]))));
			fRec4[0] = (fRec5[0] - (fSlow7 * ((fSlow8 * fRec4[2]) + (fSlow11 * fRec4[1]))));
			float fTemp3 = std::max<float>(-1.0f, std::min<float>(1.0f, (fSlow3 * (fRec4[2] + (fRec4[0] + (2.0f * fRec4[1]))))));
			fRec7[0] = ((fSlow13 * fVec1[1]) - (fSlow5 * ((fSlow6 * fRec7[1]) - (fSlow1 * fTemp1))));
			fRec6[0] = (fRec7[0] - (fSlow7 * ((fSlow8 * fRec6[2]) + (fSlow11 * fRec6[1]))));
			output1[i] = FAUSTFLOAT(((fTemp3 * (1.0f - (0.333333343f * BassEnhance_dsp_faustpower2_f(fTemp3)))) + (fSlow7 * (((fSlow10 * fRec6[0]) + (fSlow12 * fRec6[1])) + (fSlow10 * fRec6[2])))));
			fVec0[1] = fVec0[0];
			fVec1[1] = fVec1[0];
			fRec1[1] = fRec1[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec3[1] = fRec3[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec5[1] = fRec5[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec7[1] = fRec7[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
		}
	}

};

#endif
