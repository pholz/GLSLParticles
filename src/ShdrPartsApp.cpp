#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

#include "cinder/Rand.h"
#include "cinder/Surface.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Perlin.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"

#include "cinderSyphon.h"

#include "Resources.h"

#include "OscListener.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define WIDTH 512
#define HEIGHT 512
#define PARTICLES 200

class ShdrPartsApp : public AppNative
{

public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void initFbo();
	void prepareSettings(Settings *settings);
	void keyDown(KeyEvent event);
	void drawText();
	
private:
	int m_pos;
	int m_bufferIn;
	int m_bufferOut;
	int m_frameCtr;
	
	bool m_drawTextures;
	bool m_isFullScreen;
	bool m_createParticles;
	
	Perlin m_perlin;
	
	Vec3f m_vertPos;
	
	gl::VboMesh m_vbo;
	gl::Fbo m_fbo[2];
	gl::Fbo m_fboSy;
	
	gl::GlslProg m_shdrPos;
	gl::GlslProg m_shdrVel;
	gl::GlslProg m_shdrDbg;
	
	gl::Texture m_texPos;
	gl::Texture m_texVel;
	gl::Texture m_texInfo;
	gl::Texture m_texNoise;
	gl::Texture m_texSprite;
	gl::TextureRef m_texSyRef;
	
	syphonClient m_clientSyphon; //our syphon client
	syphonServer m_srvSyphon;
	
	osc::Listener m_listener;
	
	float m_parts_speed;
};

void ShdrPartsApp::initFbo()
{
	m_pos = 0;
	m_bufferIn = 0;
	m_bufferOut = 1;
	
	m_fbo[0].bindFramebuffer();
	m_fbo[1].bindFramebuffer();
	
	// POS
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	
	gl::setMatricesWindow(m_fbo[0].getSize());
	gl::setViewport(m_fbo[0].getBounds());
	
	glClearColor(.0f, .0f, .0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_texPos.enableAndBind();
	gl::draw(m_texPos, m_fbo[0].getBounds());
	m_texPos.unbind();
	
	// VEL
	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_texVel.enableAndBind();
	gl::draw(m_texVel, m_fbo[0].getBounds());
	m_texVel.unbind();
	
	// INFO
	glDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	m_texInfo.enableAndBind();
	gl::draw(m_texInfo, m_fbo[0].getBounds());
	m_texInfo.unbind();
	
	m_fbo[0].unbindFramebuffer();
	m_fbo[1].unbindFramebuffer();
	
	m_texInfo.disable();
	m_texVel.disable();
	m_texPos.disable();
}

void ShdrPartsApp::setup()
{
	gl::clear();
	
	try {
		m_shdrPos = gl::GlslProg(ci::app::loadResource(POS_VS), ci::app::loadResource(POS_FS));
		m_shdrVel = gl::GlslProg(ci::app::loadResource(VEL_VS), ci::app::loadResource(VEL_FS));
		m_shdrDbg = gl::GlslProg(ci::app::loadResource(DBG_VS), ci::app::loadResource(DBG_FS));
	}
	catch( gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
	catch( ... ) {
		std::cout << "Unable to load shader" << std::endl;
	}
	
	m_drawTextures = false;
	m_isFullScreen = false;
	m_frameCtr = 0;
	
	m_perlin = Perlin(32, clock() * .1f);
	
	Surface32f posSurf(PARTICLES, PARTICLES, true);
	Surface32f velSurf(PARTICLES, PARTICLES, true);
	Surface32f infoSurf(PARTICLES, PARTICLES, true);
	Surface32f noiseSurf(PARTICLES, PARTICLES, true);
	
	Surface32f::Iter iter = posSurf.getIter();
	
	float winW = getWindowWidth();
	float winH = getWindowHeight();
	
	while (iter.line())
	{
		while (iter.pixel())
		{
		
			m_vertPos = Vec3f(Rand::randFloat(winW) / winW, Rand::randFloat(winH) / winH, .0f);
			
			
			float nX = iter.x() * .005f;
			float nY = iter.y() * .005f;
			float nZ = app::getElapsedSeconds() * 0.1f;
			Vec3f v(nX, nY, nZ);
			float noise = m_perlin.fBm(v);
			float angle = noise * 15.0f;
			Vec2f vel(Rand::randFloat(-.0005f, .0005f), Rand::randFloat(-.0005f, .0005f));
			
			noiseSurf.setPixel(iter.getPos(),
				ColorA(cos(angle) * Rand::randFloat(.00005f,.0002f),
					  sin(angle) * Rand::randFloat(.00005f,.0002f),
					  0.0f, 1.0f ));
			
			posSurf.setPixel(iter.getPos(),
				ColorA(m_vertPos.x, m_vertPos.y, m_vertPos.z, Rand::randFloat(.00005f, .0002f)));
							 
			velSurf.setPixel(iter.getPos(), Color(vel.x, vel.y, Rand::randFloat(.01f, 1.00f)));
			infoSurf.setPixel(iter.getPos(), ColorA(Rand::randFloat(.007f, 1.0f), 1.0f, .0f, 1.00f));
		}
	}
	
	gl::Texture::Format tFormat;
	
	//tFormat.setInternalFormat(GL_RGBA16F_ARB);
	gl::Texture::Format tFormatSmall;
	//tFormat.setInternalFormat(GL_RGBA8);
	
	m_texSprite = gl::Texture(loadImage(loadResource("cross.png")), tFormatSmall);
	
	GLenum interp = GL_LINEAR;
	
	m_texNoise = gl::Texture(noiseSurf, tFormatSmall);
	m_texNoise.setWrap(GL_REPEAT, GL_REPEAT);
	m_texNoise.setMinFilter(interp);
	m_texNoise.setMagFilter(interp);
	
	m_texVel = gl::Texture(velSurf, tFormat);
	m_texVel.setWrap(GL_REPEAT, GL_REPEAT);
	m_texVel.setMinFilter(GL_NEAREST);
	m_texVel.setMagFilter(GL_NEAREST);
	
	m_texPos = gl::Texture(posSurf, tFormat);
	m_texPos.setWrap(GL_REPEAT, GL_REPEAT);
	m_texPos.setMinFilter(GL_NEAREST);
	m_texPos.setMagFilter(GL_NEAREST);
	
	m_texInfo = gl::Texture(infoSurf, tFormatSmall);
	m_texInfo.setWrap(GL_REPEAT, GL_REPEAT);
	m_texInfo.setMinFilter(GL_NEAREST);
	m_texInfo.setMagFilter(GL_NEAREST);
	
	gl::Fbo::Format format;
	format.enableDepthBuffer(false);
	format.enableColorBuffer(true, 3);
	format.setMinFilter(GL_NEAREST);
	format.setMagFilter(GL_NEAREST);
	format.setWrap(GL_CLAMP, GL_CLAMP);
	format.setColorInternalFormat(GL_RGBA16F_ARB);
	
	
	m_fbo[0] = gl::Fbo(PARTICLES, PARTICLES, format);
	
	m_fbo[1] = gl::Fbo(PARTICLES, PARTICLES, format);
	
	m_fboSy = gl::Fbo(1024, 1024, format);
	
	initFbo();
	
	vector<Vec2f> texCoords;
	vector<Vec3f> vertCoords, normCoords;
	vector<uint32_t> indices;
	
	gl::VboMesh::Layout layout;
	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();
	
	m_vbo = gl::VboMesh(PARTICLES*PARTICLES, PARTICLES*PARTICLES, layout, GL_POINTS);
	
	for (int x = 0; x < PARTICLES; x++)
	{
		for (int y = 0; y < PARTICLES; y++)
		{
			indices.push_back(x * PARTICLES + y);
			texCoords.push_back(Vec2f((float)x / (float)PARTICLES, (float)y / (float)PARTICLES));
		}
	}
	
	m_vbo.bufferIndices(indices);
	m_vbo.bufferTexCoords2d(0, texCoords);
	
	m_clientSyphon.setup();
    
	// in order for this to work, you must run simple server which is a syphon test application
    // feel free to change the app and server name for your specific case
    m_clientSyphon.set("qtz1", "Quartz Composer");
    
    m_clientSyphon.bind();
	
	m_srvSyphon.setName("parts");
	
	
	m_listener.setup(5991);
	
	m_parts_speed = .0f;
	
	m_texSyRef = gl::Texture::create(1024, 1024);
	
}

void ShdrPartsApp::mouseDown( MouseEvent event )
{
}

void ShdrPartsApp::keyDown( KeyEvent event)
{
    
	if (event.getChar() == 't')
	{
		m_drawTextures = !m_drawTextures;
	}
	else if (event.getChar() == 'f')
	{
		setFullScreen(!isFullScreen());
	}
	else if (event.getChar() == event.KEY_SPACE)
	{
		m_createParticles = true;
	}
}

void ShdrPartsApp::update()
{
	
	// OSC
	
	while( m_listener.hasWaitingMessages() )
	{
		osc::Message message;
		m_listener.getNextMessage( &message );
		
		console() << "New message received" << std::endl;
		console() << "Address: " << message.getAddress() << std::endl;
		console() << "Num Arg: " << message.getNumArgs() << std::endl;
		for (int i = 0; i < message.getNumArgs(); i++) {
			console() << "-- Argument " << i << std::endl;
			console() << "---- type: " << message.getArgTypeName(i) << std::endl;
			if( message.getArgType(i) == osc::TYPE_INT32 ) {
				try {
					console() << "------ value: "<< message.getArgAsInt32(i) << std::endl;
				}
				catch (...) {
					console() << "Exception reading argument as int32" << std::endl;
				}
			}
			else if( message.getArgType(i) == osc::TYPE_FLOAT ) {
				try {
					//console() << "------ value: " << message.getArgAsFloat(i) << std::endl;
					
					if (message.getAddress().compare("/FromVDMX/parts_speed") == 0)
					{
						m_parts_speed = message.getArgAsFloat(i);
					}
				}
				catch (...) {
					console() << "Exception reading argument as float" << std::endl;
				}
			}
			else if( message.getArgType(i) == osc::TYPE_STRING) {
				try {
					console() << "------ value: " << message.getArgAsString(i).c_str() << std::endl;
				}
				catch (...) {
					console() << "Exception reading argument as string" << std::endl;
				}
			}
		}
	}
	
	///////
	
	m_fbo[m_bufferIn].bindFramebuffer();
	
	gl::setMatricesWindow(m_fbo[0].getSize());
	gl::setViewport(m_fbo[0].getBounds());
	GLenum buffer[3] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT };
	glDrawBuffers(3, buffer);
	
	m_fbo[m_bufferOut].bindTexture(0, 0);
	m_fbo[m_bufferOut].bindTexture(1, 1);
	m_fbo[m_bufferOut].bindTexture(2, 2);
	m_texVel.bind(3);
	m_texPos.bind(4);
	
	m_clientSyphon.bind();
	
	gl::TextureRef sytex = m_clientSyphon.getTexture();
	sytex->bind(6);
	
	m_texNoise.bind(5);
	
	m_shdrVel.bind();
	m_shdrVel.uniform("positions", 0);
	m_shdrVel.uniform("velocities", 1);
	m_shdrVel.uniform("information", 2);
	m_shdrVel.uniform("oVelocities", 3);
	m_shdrVel.uniform("oPositions", 4);
	m_shdrVel.uniform("texNoise", 5);
	m_shdrVel.uniform("texNoise2", 6);
	m_shdrVel.uniform("time", m_parts_speed);
	
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f, 0.0f); glVertex2f( 0.0f, 0.0f);
	glTexCoord2f( 0.0f, 1.0f); glVertex2f( 0.0f, PARTICLES);
	glTexCoord2f( 1.0f, 1.0f); glVertex2f( PARTICLES, PARTICLES);
	glTexCoord2f( 1.0f, 0.0f); glVertex2f( PARTICLES, 0.0f);
	glEnd();
	
	m_shdrVel.unbind();
	m_fbo[m_bufferOut].unbindTexture();
	m_texVel.unbind();
	m_texPos.unbind();
	m_texNoise.unbind();
	m_fbo[m_bufferIn].unbindFramebuffer();
	sytex->unbind();
	
	m_clientSyphon.unbind();
	
	m_bufferIn = (m_bufferIn + 1) % 2;
	m_bufferOut = (m_bufferIn + 1) % 2;
	
//	m_fboSy.bindFramebuffer();
//	gl::setMatricesWindow(m_fboSy.getSize());
//	gl::setViewport(m_fboSy.getBounds());
//	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
//	gl::TextureRef sytex2 = m_clientSyphon.getTexture();
//	gl::pushMatrices();
//	gl::draw(sytex2, Rectf(0.0, 0.0, m_fboSy.getHeight(), m_fboSy.getHeight()));
//	gl::popMatrices();
//
//	m_fboSy.unbindFramebuffer();
}

void ShdrPartsApp::draw()
{
	glEnable(GL_POINT_SPRITE);
		
	if (m_drawTextures)
	{
#if 0
		gl::setMatricesWindow(getWindowSize());
		gl::setViewport(getWindowBounds());
		gl::enableAlphaBlending();
		
		gl::disableDepthRead();
		gl::clear(ColorA(.0f, .0f, .0f, 1.0f));
		gl::color(ColorA(1.f, 1.f, 1.f, 1.f));

		gl::pushMatrices();
//		gl::scale(getWindowHeight() / (float)PARTICLES, getWindowHeight()/(float)PARTICLES, 1.0f);

		
//		m_fbo[m_bufferIn].bindTexture(0, 0);

		m_texNoise.bind(1);
		gl::TextureRef sytex = m_clientSyphon.getTexture();
		m_clientSyphon.bind();
//		err = glGetError(); console() << "glerr 0: " << err;
		sytex->bind(0);
		
		m_shdrDbg.bind();
		m_shdrDbg.uniform("dbgPos", 1);
		m_shdrDbg.uniform("dbgSy", 0);
		m_shdrDbg.uniform("scale", (float)PARTICLES);
		
	//	gl::draw(m_vbo);

		gl::drawSolidRect(Rectf(0.f, 0.f, getWindowHeight(), getWindowHeight()));
		gl::popMatrices();
		
		m_shdrDbg.unbind();
		sytex->unbind();
		m_clientSyphon.unbind();
		m_fbo[m_bufferIn].unbindTexture();
		
#endif
		
#if 1
//		m_clientSyphon.draw(0.0, 0.0, getWindowWidth(), getWindowHeight());
		gl::setMatricesWindow(getWindowSize());
		gl::setViewport(getWindowBounds());
		gl::enableAlphaBlending();
		gl::disableDepthRead();
		gl::clear(ColorA(.0f, .0f, .0f, 1.0f));
		gl::color(ColorA(1.f, 1.f, 1.f, 1.f));
		
		int err;
		
		m_clientSyphon.bind();
		gl::TextureRef sytex = m_clientSyphon.getTexture();
		sytex->bind(0);
		
		m_shdrDbg.bind();
		m_shdrDbg.uniform("dbgSy", 0);
		
		gl::pushMatrices();
		gl::draw(sytex, Rectf(0.0, 0.0, PARTICLES, PARTICLES));
		
//		glBegin(GL_QUADS);
//		glTexCoord2f( 0.0f, 0.0f); glVertex2f( 0.0f, 0.0f);
//		glTexCoord2f( 0.0f, 1.0f); glVertex2f( 0.0f, getWindowHeight());
//		glTexCoord2f( 1.0f, 1.0f); glVertex2f( getWindowHeight(), getWindowHeight());
//		glTexCoord2f( 1.0f, 0.0f); glVertex2f( getWindowHeight(), 0.0f);
//		glEnd();

		gl::popMatrices();
		
		sytex->unbind();
		m_clientSyphon.unbind();
		m_shdrDbg.unbind();

		
#endif
		
	}
	else
	{
		m_fboSy.bindFramebuffer();
		gl::setMatricesWindow(m_fboSy.getSize());
		gl::setViewport(m_fboSy.getBounds());
//		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		GLenum bfrs[1] = {GL_COLOR_ATTACHMENT0_EXT};
		glDrawBuffers(1, bfrs);
		
		
//		gl::setMatricesWindow(getWindowSize());
//		gl::setViewport(getWindowBounds());
		
		gl::clear(ColorA(.0f, .0f, .0f, 1.0f));
		gl::enableAlphaBlending();
		glBlendFunc(GL_ONE, GL_ONE);
		gl::disableDepthRead();

		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
		
		m_fbo[m_bufferIn].bindTexture(0, 0);
		m_fbo[m_bufferIn].bindTexture(1, 1);
		m_fbo[m_bufferIn].bindTexture(2, 2);
		
		m_texSprite.bind(3);
		
		
		
		m_shdrPos.bind();
		m_shdrPos.uniform("texPos", 0);
		m_shdrPos.uniform("texVel", 1);
		m_shdrPos.uniform("texInf", 2);
		m_shdrPos.uniform("texSprite", 3);
		m_shdrPos.uniform("texSy", 4);
		m_shdrPos.uniform("scale", (float)PARTICLES);
		
		gl::color(ColorA(1.f, 1.f, 1.f, 1.f));
		gl::pushMatrices();
		
//		gl::scale(getWindowHeight() / (float)PARTICLES, getWindowHeight()/(float)PARTICLES, 1.0f);
		gl::scale(1024.0 / (float)PARTICLES, 1024.0/(float)PARTICLES, 1.0f);
		
		gl::draw(m_vbo);
		
		gl::popMatrices();
		
		m_shdrPos.unbind();
		m_texSprite.unbind();
		m_fbo[m_bufferIn].unbindTexture();

		gl::disableAlphaBlending();
		
		
		m_fboSy.unbindFramebuffer();
//		gl::draw(m_fboSy.getTexture());
		
		
		*m_texSyRef = m_fboSy.getTexture();
		//gl::draw(ref);
		
		m_srvSyphon.publishTexture(m_texSyRef);
		
//		m_srvSyphon.publishScreen();
	
	}
		
//	drawText();
	
	if (getElapsedFrames() % 60 == 0)
		console() << "FPS: " << getAverageFps() << std::endl;
	
}

void ShdrPartsApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(WIDTH,HEIGHT);
    settings->setFrameRate(60.0f);
	
}

void ShdrPartsApp::drawText()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	TextLayout layout;
	layout.setFont( Font( "Arial-BoldMT", 14 ) );
	layout.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	layout.addLine( "1 Million Particles" );
	layout.addLine( " " );
	
	layout.setColor( Color( 0.9f, 0.9f, 0.9f ) );
	layout.setFont( Font( "ArialMT", 12 ) );
	
	layout.addLine("F - switch to fullscreen");
	layout.addLine("t - draw textures");
	
	char fps[50];
	sprintf(fps, "FPS: %.2f", getAverageFps());
    
	char particleCount[50];
	sprintf(particleCount, "Particles: %d", PARTICLES*PARTICLES);
	
	layout.addLine(fps);
	layout.addLine(particleCount);
    
    glEnable( GL_TEXTURE_2D );
	gl::draw(layout.render(true), Vec2f(getWindowWidth() - 150,25));
	glDisable( GL_TEXTURE_2D );
}

CINDER_APP_NATIVE( ShdrPartsApp, RendererGl )
