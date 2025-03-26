#include "ofApp.h"
//Dev for; PrusaSlicer Version 2.4.0-beta2+win64



//--------------------------------------------------------------
void ofApp::setup(){
	logoImg.loadImage("logo.png");

	mesh.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

	currentSrcFile = "";

	menu.setup("Menu");
	menu.setSize(600, 500);
	menu.setPosition(0, 0);
	menu.setDefaultWidth(600);

	geometricSettings.setName("Geometric settings");
	geometricSettings.add(armSpeed.set("Arm speed [m/s]", 0.1, 0.01, 0.2));
	armSpeed.addListener(this,&ofApp::calculateExtrusionData);
	geometricSettings.add(toolOffsetZ.set("Tool offset Z", 0, 0, 1000));
	geometricSettings.add(bedOffsetZ.set("Bed offset Z", 0, 0, 500));
	geometricSettings.add(printCenter.set("Print center", ofVec3f(0, 0, 0), ofVec3f(-2000, -2000, 0), ofVec3f(2500, 2500, 400)));
	

	programSettings.setName("Program control");
	programSettings.add(srcFileOpen.set("Open gcode file",false));
	programSettings.add(procesDataButton.set("Create Kuka code", false));
	programSettings.add(srcFileLabel.set("File: ",currentSrcFile));
	srcFileLabel.setSerializable(false);

	extruderSettings.setName("Extruder settings");
	extruderSettings.add(layerHeight.set("Layer height [mm]",2,0.5,15));
	layerHeight.addListener(this, &ofApp::calculateExtrusionData);
	extruderSettings.add(extrusionWidth.set("Extrusion width [mm]",3,2,15));
	extrusionWidth.addListener(this, &ofApp::calculateExtrusionData);
	extruderSettings.add(extruderVolumeRev.set("Volume [cm3/rev]", 1.26, 0.1, 3));
	extruderVolumeRev.addListener(this, &ofApp::calculateExtrusionData);
	extruderSettings.add(calculatedRpm.set("Extruder [rpm]",""));
	calculatedRpm.setSerializable(false);
	extruderSettings.add(calculatedAnalog.set("Extruder [0-1]", ""));
	calculatedAnalog.setSerializable(false);
	extruderSettings.add(calculatedFC.set("Extruder [FC]",""));
	calculatedFC.setSerializable(false);
	
	menu.add(programSettings);
	menu.add(geometricSettings);
	//menu.getGroup("Geometric settings").minimize();
	menu.add(extruderSettings);
	//menu.getGroup("Extruder settings").minimize();
	
	
	menu.loadFromFile(ofxPanelDefaultFilename);
	srcFileOpen.addListener(this, &ofApp::openSrcFile);
	procesDataButton.addListener(this, &ofApp::processData);

}

void ofApp::calculateExtrusionData(float &sender) {
	float eHeight = layerHeight.get();
	float eWidth = extrusionWidth.get();

	float mSpeed = armSpeed.get();

	//Calculate surface of extrusion over Z-X, which is a slot. Take rectangular volume, subtract the round corners
	//by subtracting round layer height from square layer height. 

	float eSurface = (eHeight * eWidth) - ((eHeight * eHeight) - (PI * ((eHeight / 2) * (eHeight / 2))));
	
	std::cout << "Extrusion cross-section surface: " << eSurface << "mm2" << std::endl;

	float dMinute = armSpeed * 60.0f;

	std::cout << "Machine travel per minute [m/min]" << dMinute << std::endl;

	float eVolumeMinute = (dMinute * 1000.0f) * eSurface;

	std::cout << "Extrusion volume at max speed [mm3/min]: " << eVolumeMinute << std::endl;

	eVolumeMinute = eVolumeMinute / 1000.0f;
	
	std::cout << "Extrusion volume at max speed [cm3/min]: " << eVolumeMinute << std::endl;

	float calculatedFeed = eVolumeMinute / extruderVolumeRev.get();

	std::cout << "Calculated extruder RPM: " << calculatedFeed << std::endl;
	std::cout << "Calculated analog target [0-1]: " << ofMap(calculatedFeed,0,150,0,1,true) << std::endl;
	std::cout << "Calculated FLOWCORRECTION ( Analog [0-1] =(FC * $VEL.CP) )" << ofMap(calculatedFeed, 0, 150, 0, 1, true) / armSpeed.get() << std::endl;

	calculatedRpm.set(ofToString(calculatedFeed, 4));
	calculatedAnalog.set(ofToString(ofMap(calculatedFeed, 0, 150, 0, 1, true), 4));
	calculatedFC.set(ofToString(ofMap(calculatedFeed, 0, 150, 0, 1, true) / armSpeed.get(),4));

}

void ofApp::openSrcFile(bool& sender){

	ofFileDialogResult result = ofSystemLoadDialog("Open gcode file", false);
	if (result.bSuccess) {
		std::cout << result.fileName << " :::: " << result.filePath << std::endl;
		currentSrcFile = result.filePath;
		srcFileLabel.set(result.fileName);
	}

}

void ofApp::processData(bool& sender) {
	if (!currentSrcFile.empty()) {
		mesh.clear();
		int meshIndex = 0;

		std::vector<std::string> nCommands;
		std::vector<std::string> basicHeader;
		int countFiles = 0;

		basicHeader.push_back("DEF ofgen()");

		basicHeader.push_back("GLOBAL INTERRUPT DECL 3 WHEN $STOPMESS==TRUE DO IR_STOPM ( )");
		basicHeader.push_back("INTERRUPT ON 3");
		basicHeader.push_back("BAS (#INITMOV,0 )");

		basicHeader.push_back("ANOUT ON AO_EXTRUDER_RPM = FLOW_CORRECTION * $VEL_ACT +0.0 DELAY=-0.2");
		basicHeader.push_back("FLOW_CORRECTION = "+ calculatedFC.get());

		basicHeader.push_back("$BWDSTART = FALSE");
		basicHeader.push_back("PDAT_ACT = {VEL 15,ACC 100,APO_DIST 50}");
		basicHeader.push_back("BAS(#PTP_DAT)");
		basicHeader.push_back("FDAT_ACT = {TOOL_NO 6,BASE_NO 0,IPO_FRAME #BASE}");
		basicHeader.push_back("BAS(#FRAMES)");
		
		basicHeader.push_back("BAS (#VEL_PTP,15)");
		basicHeader.push_back("PTP  {A1 5,A2 -90,A3 100,A4 5,A5 -10,A6 -5,E1 0,E2 0,E3 0,E4 0}");
		
		basicHeader.push_back("$VEL.CP="+ofToString(armSpeed,2));
		basicHeader.push_back("$ADVANCE=3");
		
		
		

		ofBuffer gBuf = ofBufferFromFile(currentSrcFile,false);
		float exportLastE = false;

		float lastX = 0.0f;
		float lastY = 0.0f;
		float lastZ = 0.0f;
		bool lastE = false;

		while (!gBuf.isLastLine()) {
			std::string cLine = gBuf.getNextLine();
			bool hasMove = cLine.find("G1") != std::string::npos;
			
			if (hasMove) {

				bool hasX = cLine.find("X") != std::string::npos;
				bool hasY = cLine.find("Y") != std::string::npos;
				bool hasZ = cLine.find("Z") != std::string::npos;
				bool hasE = cLine.find("E") != std::string::npos;

				float cxVal, cyVal, czVal;

				if (hasX) {
					size_t sPos = cLine.find("X");
					size_t ePos = cLine.find(" ",sPos);

					std::string nString = cLine.substr(sPos + 1, ePos - sPos - 1);
					cxVal = std::stof(nString);
				}
				if (hasY) {
					size_t sPos = cLine.find("Y");
					size_t ePos = cLine.find(" ", sPos);

					std::string nString = cLine.substr(sPos + 1, ePos - sPos - 1);
					cyVal = std::stof(nString);
				}
				if (hasZ) {
					size_t sPos = cLine.find("Z");
					size_t ePos = cLine.find(" ", sPos);

					std::string nString = cLine.substr(sPos + 1, ePos - sPos - 1);
					czVal = std::stof(nString);
				}

				

				//Handle extruder here------------------------------------------------------------------------------------------
				if (hasE != lastE) {
					if (!lastE) {
						
						nCommands.push_back("TRIGGER WHEN DISTANCE = 0 DELAY = 0 DO O_EXTRUDER_START = TRUE");
					}
					if (lastE) {
						
						nCommands.push_back("TRIGGER WHEN DISTANCE = 0 DELAY = 0 DO O_EXTRUDER_START = FALSE");
					}

					lastE = hasE;
				}

				if (hasZ && !hasX && !hasY && !hasE) { //Gcode Z move
					
					if (lastX == 0.0f && lastY == 0.0f) { //Gcode first Z move, use print center
						
						cxVal = printCenter.get().x;
						cyVal = printCenter.get().y;

						//This assumes this only happens once!!!
						nCommands.push_back("PTP {X " + ofToString(cxVal) + ", Y " + ofToString(cyVal) + ", Z " + ofToString(czVal + bedOffsetZ.get() + toolOffsetZ.get()) + ", A 0, B 90, C 0, E1 0, E2 0, E3 0, E4 0, S 'B 110'} C_PTP");
					}
					else {
						cxVal = lastX;
						cyVal = lastY;

						nCommands.push_back("LIN{ X "+ofToString(cxVal + printCenter.get().x)+", Y " + ofToString(cyVal + printCenter.get().y) + ", Z " + ofToString(czVal + bedOffsetZ.get() + toolOffsetZ.get()) + ", A 0, B 90, C 0 } C_DIS");
					}
					
					

					lastX = cxVal;
					lastY = cyVal;
					lastZ = czVal;
					
				}
				else if (hasX && hasY && !hasZ && lastZ != 0.0f) { //Gcode XY-planar move
					
					czVal = lastZ;

					nCommands.push_back("LIN{ X " + ofToString(cxVal + printCenter.get().x) + ", Y " + ofToString(cyVal + printCenter.get().y) + ", Z " + ofToString(czVal + bedOffsetZ.get() + toolOffsetZ.get()) + ", A 0, B 90, C 0 } C_DIS");

					lastX = cxVal;
					lastY = cyVal;
					lastZ = czVal;

					mesh.addVertex(ofPoint((cxVal + printCenter.get().x)/10, (cyVal + printCenter.get().y)/10, (czVal + bedOffsetZ.get() + toolOffsetZ.get())/10));
					if (hasE) mesh.addIndex(meshIndex);
					meshIndex++;

				}
				else if (hasX && hasY && hasZ) { //Gcode 3-axis Move

					
					nCommands.push_back("LIN{ X " + ofToString(cxVal + printCenter.get().x) + ", Y " + ofToString(cyVal + printCenter.get().y) + ", Z " + ofToString(czVal + bedOffsetZ.get() + toolOffsetZ.get(),3) + ", A 0, B 90, C 0 } C_DIS");

					lastX = cxVal;
					lastY = cyVal;
					lastZ = czVal;

					mesh.addVertex(ofPoint((cxVal + printCenter.get().x) / 10, (cyVal + printCenter.get().y) / 10, (czVal + bedOffsetZ.get() + toolOffsetZ.get()) / 10));
					if (hasE) mesh.addIndex(meshIndex);
					meshIndex++;

				}
				else if (!hasX && !hasY && !hasZ && !hasE) { //Gcode Feedrate command, igonore

					

				}
				
				
				

			}

			if (nCommands.size() > 19900*1.5) {

				nCommands.push_back("END");


				ofFile nFile = ofFile(ofToDataPath("kukaConv_" + ofToString(countFiles) + ".src", true));
				if (nFile.exists()) {
					nFile.remove() ? std::cout << "Old conversion removed!" << std::endl : std::cout << "Couldnt remove old conversion!" << std::endl;
				}

				nFile.create() ? std::cout << "New conversion created!" << std::endl : std::cout << "Couldnt create new conversion!" << std::endl;

				if (nFile.open(ofToDataPath("kukaConv_" + ofToString(countFiles) + ".src", true), ofFile::Mode::Append, false)) {

					for (int i = 0; i < basicHeader.size(); i++) {
						nFile << basicHeader[i] << std::endl;
					}

					//Check last extruder state and switch on and off. 
					if (exportLastE) {
						nCommands.push_back("O_EXTRUDER_START = FALSE");
						nCommands.push_back("TRIGGER WHEN DISTANCE = 0 DELAY = 0 DO O_EXTRUDER_START = TRUE");

						std::cout << "Intercepted extrusion break; custom restart" << std::endl;

					}

					for (int i = 0; i < nCommands.size(); i++) {
						nFile << nCommands[i] << std::endl;
					}
					std::cout << "File written!" << std::endl;

					nFile.close();

				}
				else {

					std::cout << "Couldnt open file, data lost!" << std::endl;

				}
				
				exportLastE = lastE;

				countFiles++;
				nCommands.clear();

			}
			
		}

		nCommands.push_back("END");


		ofFile nFile = ofFile(ofToDataPath("kukaConv_" + ofToString(countFiles) + ".src",true));
		if (nFile.exists()) {
			nFile.remove() ? std::cout << "Old conversion removed!" << std::endl : std::cout << "Couldnt remove old conversion!" << std::endl;
		}

		nFile.create() ? std::cout << "New conversion created!" << std::endl : std::cout << "Couldnt create new conversion!" << std::endl;
			
		if (nFile.open(ofToDataPath("kukaConv_" + ofToString(countFiles) + ".src", true),ofFile::Mode::Append,false)) {

			for (int i = 0; i < basicHeader.size(); i++) {
				nFile << basicHeader[i] << std::endl;
			}

			for (int i = 0; i < nCommands.size(); i++) {
				nFile << nCommands[i] << std::endl;
			}
			std::cout << "File written!" << std::endl;

			nFile.close();

		}
		else {

			std::cout << "Couldnt open file, data lost!" << std::endl;

		}
	}
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(ofColor(255, 255, 255));
	menu.draw();
	
	cam.begin();

	//ofPushMatrix();
	//ofTranslate(1100, (ofGetWindowHeight() / 2));

	ofNoFill();
	ofSetCircleResolution(100);
	ofDrawCircle(0, 0, 296);
	ofDrawCircle(0, 0, 127);
	
	ofFill();
	ofSetColor(ofColor(0, 0, 255));
	
	ofDrawCircle(printCenter.get().x/10,printCenter.get().y/10,5);

	ofDrawBitmapStringHighlight("Y-", ofVec2f(0, 296), ofColor(0, 255, 0), ofColor(0));
	ofDrawBitmapStringHighlight("Y+", ofVec2f(0, -296), ofColor(0, 255, 0), ofColor(0));

	ofDrawBitmapStringHighlight("X+", ofVec2f(296, 0), ofColor(255, 0, 0), ofColor(0));
	ofDrawBitmapStringHighlight("X-", ofVec2f(-296, 0), ofColor(255, 0, 0), ofColor(0));

	//ofPopMatrix();
	mesh.draw();

	cam.end();

	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255);
	logoImg.draw(10, ofGetWindowHeight()-(logoImg.getHeight()/3),logoImg.getWidth()/3,logoImg.getHeight()/3);
	ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
