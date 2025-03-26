#pragma once

#include "ofMain.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void openSrcFile(bool &sender);
		void processData(bool& sender);
		void calculateExtrusionData(float &sender);

		ofxPanel menu;
		ofParameterGroup geometricSettings;
		ofParameter<ofVec2f> printCenter;
		ofParameter<float> toolOffsetZ;
		ofParameter<float> bedOffsetZ;
		ofParameter<float> armSpeed;

		ofParameterGroup programSettings;
		ofParameter<bool> srcFileOpen;
		ofParameter<std::string> srcFileLabel;
		ofParameter<bool> procesDataButton;

		ofParameterGroup extruderSettings;
		ofParameter<float> layerHeight;
		ofParameter<float> extrusionWidth;
		ofParameter<float> extruderVolumeRev;
		ofParameter<std::string> calculatedRpm;
		ofParameter<std::string> calculatedAnalog;
		ofParameter<std::string> calculatedFC;

		std::string currentSrcFile;

		ofImage logoImg;

		ofEasyCam cam;
		ofMesh mesh;
		
};
