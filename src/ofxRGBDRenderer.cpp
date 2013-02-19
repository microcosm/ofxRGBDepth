/**
 * ofxRGBDepth addon
 *
 * Core addon for programming against the RGBDToolkit API
 * http://www.rgbdtoolkit.com
 *
 * (c) James George 2011-2013 http://www.jamesgeorge.org
 *
 * Developed with support from:
 *      Frank-Ratchy STUDIO for Creative Inquiry http://studioforcreativeinquiry.org/
 *      YCAM InterLab http://interlab.ycam.jp/en
 *      Eyebeam http://eyebeam.org
 */

#include "ofxRGBDRenderer.h"

using namespace ofxCv;
using namespace cv;

ofxRGBDRenderer::ofxRGBDRenderer(){
    
	shift.x = 0;
	shift.y = 0;
	scale.x = 1.0;
    scale.y = 1.0;
    
    flipTexture = false;
    
    nearClip = 1;
	edgeClip = 50;
	simplify = ofVec2f(0,0);
    
	farClip = 6000;
    //meshRotate = ofVec3f(0,0,0);
    
	hasDepthImage = false;
	hasRGBImage = false;
    
	mirror = false;
	calibrationSetup = false;
    
    addColors = false;
    currentDepthImage = NULL;
    useTexture = true;
    meshGenerated = false;
}

ofxRGBDRenderer::~ofxRGBDRenderer(){
    
}

bool ofxRGBDRenderer::setup(string calibrationDirectory){
	
	if(!ofDirectory(calibrationDirectory).exists()){
		ofLogError("ofxRGBDGPURenderer --- Calibration directory doesn't exist: " + calibrationDirectory);
		return false;
	}
	return setup(calibrationDirectory+"/rgbCalib.yml", calibrationDirectory+"/depthCalib.yml",
                 calibrationDirectory+"/rotationDepthToRGB.yml", calibrationDirectory+"/translationDepthToRGB.yml");
}

bool ofxRGBDRenderer::setup(string rgbIntrinsicsPath,
                            string depthIntrinsicsPath, string rotationPath, string translationPath)
{
    
    depthCalibration.load(depthIntrinsicsPath);
    rgbCalibration.load(rgbIntrinsicsPath);

    loadMat(rotationDepthToRGB, rotationPath);
    loadMat(translationDepthToRGB, translationPath);

    depthToRGBView = ofxCv::makeMatrix(rotationDepthToRGB, translationDepthToRGB);

    ofPushView();
    rgbCalibration.getDistortedIntrinsics().loadProjectionMatrix();
    glGetFloatv(GL_PROJECTION_MATRIX, rgbProjection.getPtr());
    ofPopView();

    ofPushView();
    depthCalibration.getDistortedIntrinsics().loadProjectionMatrix();
    glGetFloatv(GL_PROJECTION_MATRIX, depthProjection.getPtr());
    ofPopView();

    rgbMatrix = (depthToRGBView * rgbProjection);

    //	Point2d fov = depthCalibration.getUndistortedIntrinsics().getFov();
    //	fx = tanf(ofDegToRad(fov.x) / 2) * 2;
    //	fy = tanf(ofDegToRad(fov.y) / 2) * 2;
    //	fx = depthCalibration.getUndistortedIntrinsics().getCameraMatrix().at<double>(0,0);
    //	fy = depthCalibration.getUndistortedIntrinsics().getCameraMatrix().at<double>(1,1);
    //	principalPoint = depthCalibration.getUndistortedIntrinsics().getPrincipalPoint();
    //	imageSize = depthCalibration.getUndistortedIntrinsics().getImageSize();

    fx = depthCalibration.getDistortedIntrinsics().getCameraMatrix().at<double>(0,0);
    fy = depthCalibration.getDistortedIntrinsics().getCameraMatrix().at<double>(1,1);
    principalPoint = depthCalibration.getDistortedIntrinsics().getPrincipalPoint();
    imageSize = depthCalibration.getDistortedIntrinsics().getImageSize();

    //  cout << "successfully loaded calibration: fx + fy is " << fx << " " << fy  << endl;
    //	cout << "RGB Camera Matrix is " << rgbCalibration.getDistortedIntrinsics().getCameraMatrix() << endl;
    //	cout << "RGB Distortion coefficients " << rgbCalibration.getDistCoeffs() << endl;
    //	cout << "Depth Camera Matrix is " << depthCalibration.getDistortedIntrinsics().getCameraMatrix() << endl;
    //	cout << "Depth Distortion coefficients " << depthCalibration.getDistCoeffs() << endl;
    //	cout << "RGB->Depth rotation " << rotationDepthToRGB << endl;
    //	cout << "RGB->Depth translation " << translationDepthToRGB << endl;
    //	cout << "RGB Aspect Ratio " << rgbCalibration.getDistortedIntrinsics().getAspectRatio() << endl;
    //	cout << "RGB Focal Length " << rgbCalibration.getDistortedIntrinsics().getFocalLength() << endl;

    calibrationSetup = true;

    if(!meshGenerated){
        setSimplification(ofVec2f(1,1));
    }
    
    return true;
}

//-----------------------------------------------
ofVec2f ofxRGBDRenderer::getSimplification(){
	return simplify;
}

//-----------------------------------------------
void ofxRGBDRenderer::drawMesh(){
    draw(OF_MESH_FILL);
}

//-----------------------------------------------
void ofxRGBDRenderer::drawPointCloud(){
    draw(OF_MESH_POINTS);
}

//-----------------------------------------------
void ofxRGBDRenderer::drawWireFrame(){
    draw(OF_MESH_WIREFRAME);
}

//-----------------------------------------------
void ofxRGBDRenderer::setDepthImage(ofShortPixels& pix){
    currentDepthImage = &pix;
	hasDepthImage = true;
}

//-----------------------------------------------
void ofxRGBDRenderer::setRGBTexture(ofBaseHasTexture& tex){
	currentRGBImage = &tex;
	hasRGBImage = true;
}

//-----------------------------------------------
ofBaseHasTexture& ofxRGBDRenderer::getRGBTexture() {
    return *currentRGBImage;
}

//-----------------------------------------------
void ofxRGBDRenderer::setXYShift(ofVec2f newShift){
    shift = newShift;
}

//-----------------------------------------------
void ofxRGBDRenderer::setXYScale(ofVec2f newScale){
    scale = newScale;    
}

//-----------------------------------------------
void ofxRGBDRenderer::drawProjectionDebug(bool showDepth, bool showRGB, float rgbTexturePosition){
    ofPushStyle();
    
	glEnable(GL_DEPTH_TEST);
	if(showRGB){
		ofPushMatrix();
		ofSetColor(255);
		rgbMatrix = (depthToRGBView * rgbProjection);
		ofScale(1,-1,1);
		glMultMatrixf(rgbMatrix.getInverse().getPtr());
		
		ofNoFill();
		ofSetColor(255,200,10);
		ofBox(1.99f);
		
		//draw texture
		if(rgbTexturePosition > 0){
			ofSetColor(255);
			ofTranslate(0, 0, 1.0 - powf(1-rgbTexturePosition, 2.0));
            currentRGBImage->getTextureReference().draw(1, 1, -2, -2);
		}
		ofPopMatrix();
	}
	
	if(showDepth){
		ofPushMatrix();
		ofScale(-1,1,-1);
		ofNoFill();
		ofSetColor(10,200,255);
		glMultMatrixf(depthProjection.getInverse().getPtr());
		ofBox(1.99f);
		ofPopMatrix();
	}
	
	glDisable(GL_DEPTH_TEST);
    ofPopStyle();
}

ofMesh& ofxRGBDRenderer::getMesh(){
    return mesh;
}

Calibration& ofxRGBDRenderer::getDepthCalibration(){
	return depthCalibration;
}

Calibration& ofxRGBDRenderer::getRGBCalibration(){
	return rgbCalibration;
}

ofMatrix4x4& ofxRGBDRenderer::getRGBMatrix(){
	return rgbMatrix;
}

ofMatrix4x4& ofxRGBDRenderer::getDepthToRGBTransform(){
	return depthToRGBView;
}
