#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ShdrPartsApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void ShdrPartsApp::setup()
{
}

void ShdrPartsApp::mouseDown( MouseEvent event )
{
}

void ShdrPartsApp::update()
{
}

void ShdrPartsApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( ShdrPartsApp, RendererGl )
