#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <QTime>
#include <ngl/NGLStream.h>


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;

const static int FBWIDTH=1024;
const static int FBHEIGHT=720;

NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0;
  m_spinYFace=0;
  m_wireframe=GL_FILL;
  m_debug=true;
  m_debugMode=0;
  setTitle("Basic Deferred Shading Demo");
  m_fps=0;
  m_frames=0;
  m_timer.start();

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
  glDeleteFramebuffers(1,&m_gbuffer);
  glDeleteTextures(1,&m_colourTexID);
  glDeleteTextures(1,&m_pointTexID);
  glDeleteTextures(1,&m_normalTexID);
}


void NGLScene::resizeGL(int _w , int _h)
{
  m_project=ngl::perspective(45.0f,(float)_w/_h,0.05f,350.0f);
  m_width  = static_cast<int>( _w * devicePixelRatio() );
  m_height = static_cast<int>( _h * devicePixelRatio() );

}
void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0.0f,2.0f,5.0f);
  ngl::Vec3 to(0.0f,0.0f,0.0f);
  ngl::Vec3 up(0.0f,1.0f,0.0f);
  // now load to our new camera
  m_view=ngl::lookAt(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project=ngl::perspective(45.0f,(float)float(width()/height()),0.05f,350.0f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
   // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  // we are creating a shader for Pass 1
  shader->createShaderProgram("Pass1");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("Pass1Vertex",ngl::ShaderType::VERTEX);
  shader->attachShader("Pass1Fragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("Pass1Vertex","shaders/Pass1Vert.glsl");
  shader->loadShaderSource("Pass1Fragment","shaders/Pass1Frag.glsl");
  // compile the shaders
  shader->compileShader("Pass1Vertex");
  shader->compileShader("Pass1Fragment");
  // add them to the program
  shader->attachShaderToProgram("Pass1","Pass1Vertex");
  shader->attachShaderToProgram("Pass1","Pass1Fragment");
//  shader->bindFragDataLocation("Pass1",0,"pointDeferred");
//  shader->bindFragDataLocation("Pass1",1,"normalDeferred");
//  shader->bindFragDataLocation("Pass1",2,"colourDeferred");
//  shader->bindFragDataLocation("Pass1",3,"shadingDeferred");

  shader->linkProgramObject("Pass1");
  shader->use("Pass1");

  // we are creating a shader for Pass 2
  shader->createShaderProgram("Pass2");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("Pass2Vertex",ngl::ShaderType::VERTEX);
  shader->attachShader("Pass2Fragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("Pass2Vertex","shaders/Pass2Vert.glsl");
  shader->loadShaderSource("Pass2Fragment","shaders/Pass2Frag.glsl");
  // compile the shaders
  shader->compileShader("Pass2Vertex");
  shader->compileShader("Pass2Fragment");
  // add them to the program
  shader->attachShaderToProgram("Pass2","Pass2Vertex");
  shader->attachShaderToProgram("Pass2","Pass2Fragment");
  shader->linkProgramObject("Pass2");
  shader->use("Pass2");
  shader->setUniform("pointTex",0);
  shader->setUniform("normalTex",1);
  shader->setUniform("colourTex",2);
  shader->setUniform("lightPassTex",3);
  shader->setUniform("diffusePassTex",4);

  // we are creating a shader for Debug pass
  shader->createShaderProgram("Debug");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("DebugVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("DebugFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("DebugVertex","shaders/DebugVert.glsl");
  shader->loadShaderSource("DebugFragment","shaders/DebugFrag.glsl");
  // compile the shaders
  shader->compileShader("DebugVertex");
  shader->compileShader("DebugFragment");
  // add them to the program
  shader->attachShaderToProgram("Debug","DebugVertex");
  shader->attachShaderToProgram("Debug","DebugFragment");
  shader->linkProgramObject("Debug");
  shader->use("Debug");
  shader->setUniform("pointTex",0);
  shader->setUniform("normalTex",1);
  shader->setUniform("colourTex",2);
  shader->setUniform("lightPassTex",3);
  shader->setUniform("diffusePassTex",4);

  shader->setUniform("mode",1);


  // we are creating a shader for Lighting pass
  shader->createShaderProgram("Lighting");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("LightingVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("LightingFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("LightingVertex","shaders/LightingPassVert.glsl");
  shader->loadShaderSource("LightingFragment","shaders/LightingPassFrag.glsl");
  // compile the shaders
  shader->compileShader("LightingVertex");
  shader->compileShader("LightingFragment");
  // add them to the program
  shader->attachShaderToProgram("Lighting","LightingVertex");
  shader->attachShaderToProgram("Lighting","LightingFragment");
  shader->linkProgramObject("Lighting");
  shader->use("Lighting");
  shader->setUniform("pointTex",0);
  shader->setUniform("normalTex",1);
  shader->setUniform("colourTex",2);
  shader->setUniform("lightPassTex",3);

  shader->setUniform("wh",float(width()*devicePixelRatio()),float(height()*devicePixelRatio()));

  m_text.reset(new ngl::Text(QFont("Arial",14)));
  m_text->setScreenSize(width(),height());

  m_screenQuad = new ScreenQuad("Debug");
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  prim->createSphere("sphere",0.5f,50.0f);

  prim->createSphere("lightSphere",0.1f,10.0f);
  prim->createCylinder("cylinder",0.5f,1.4f,40.0f,40.0f);

  prim->createCone("cone",0.5f,1.4f,20.0f,20.0f);

  prim->createDisk("disk",0.8f,120.0f);
  prim->createTorus("torus",0.15f,0.4f,40.0f,40.0f);
  prim->createTrianglePlane("plane",14.0f,14.0f,80.0f,80.0f,ngl::Vec3(0.0f,1.0f,0.0f));

  createFrameBuffer();
  printFrameBufferInfo(m_gbuffer);
  printFrameBufferInfo(m_lightBuffer);
  m_fpsTimer =startTimer(0);

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Pass1");
 // ngl::Mat4 view=ngl::lookAt(ngl::Vec3(0,1,5),ngl::Vec3(0,0,0),ngl::Vec3(0,1,0));
 // ngl::Mat4 proj=ngl::perspective(45,float(width()/height()),0.1,50);
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat4 M;

  M=m_mouseGlobalTX*m_transform.getMatrix();
  MV=  m_view*M;
  MVP= m_project*MV;
  shader->setUniform("MVP",MVP);
  shader->setUniform("MV",MV);
}




void NGLScene::createFrameBuffer()
{

  // create a framebuffer object this is deleted in the dtor
  int w=width()*devicePixelRatio();
  int h=height()*devicePixelRatio();

  glGenFramebuffers(1, &m_gbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer);

  // create a renderbuffer object to store depth info
  GLuint m_rboID;
  glGenRenderbuffers(1, &m_rboID);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rboID);

  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboID);

  // create a texture object
  glGenTextures(1, &m_pointTexID);
  // bind it to make it active
  glBindTexture(GL_TEXTURE_2D, m_pointTexID);
  // set params
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  // create a texture object
  glGenTextures(1, &m_normalTexID);
  // bind it to make it active
  glBindTexture(GL_TEXTURE_2D, m_normalTexID);
  // set params
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  glGenTextures(1, &m_colourTexID);
  // bind it to make it active
  glBindTexture(GL_TEXTURE_2D, m_colourTexID);
  // set params
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

  glGenTextures(1, &m_shadingTexID);
  // bind it to make it active
  glBindTexture(GL_TEXTURE_2D, m_shadingTexID);
  // set params
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);


  // The depth buffer
//	glGenTextures(1, &m_depthTex);
//	glBindTexture(GL_TEXTURE_2D, m_depthTex);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


  // attatch the texture we created earlier to the FBO
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pointTexID, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTexID, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_colourTexID, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_shadingTexID, 0);

  // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);

  // now attach a renderbuffer to depth attachment point
//  GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
//  glDrawBuffers (3, drawBuffers);


  // now we are going to create a buffer for the lighting pass

  // create a framebuffer object this is deleted in the dtor

  glGenFramebuffers(1, &m_lightBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_lightBuffer);

  // create a renderbuffer object to store depth info
  glGenRenderbuffers(1, &m_rboID);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rboID);

  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
  glGenTextures(1, &m_lightPassTexID);
  // bind it to make it active
  glBindTexture(GL_TEXTURE_2D, m_lightPassTexID);
  // set params
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);





  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightPassTexID, 0);
  GLuint lightBuffers[] = { GL_COLOR_ATTACHMENT0};
  glDrawBuffers (1, lightBuffers);

  // now got back to the default render context
  glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());


}

void NGLScene::paintGL()
{

  glBindFramebuffer (GL_FRAMEBUFFER, m_gbuffer);
  /*************************************************************************************************/
  // Geo Pass
  /*************************************************************************************************/

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
  glDrawBuffers (4, drawBuffers);

  drawScene("Pass1");
  /*************************************************************************************************/
  // now do the light pass
  /*************************************************************************************************/
/*  glBindFramebuffer (GL_FRAMEBUFFER, m_lightBuffer);
  GLenum lightBuffers[] = { GL_COLOR_ATTACHMENT0};
  glDrawBuffers (1, lightBuffers);

  glClear (GL_COLOR_BUFFER_BIT);

  // map the texture buffers so light pass can read them
 glActiveTexture (GL_TEXTURE0);
 glBindTexture (GL_TEXTURE_2D, m_pointTexID);
 glActiveTexture (GL_TEXTURE1);
 glBindTexture (GL_TEXTURE_2D, m_normalTexID);
 glActiveTexture (GL_TEXTURE2);
 glBindTexture (GL_TEXTURE_2D, m_colourTexID);
 glActiveTexture (GL_TEXTURE3);
 glBindTexture (GL_TEXTURE_2D, m_shadingTexID);
// glActiveTexture (GL_TEXTURE4);
// glBindTexture (GL_TEXTURE_2D, m_lightPassTexID);



 glGenerateMipmap(GL_TEXTURE_2D);

 glClear(GL_COLOR_BUFFER_BIT);

// renderLightPass("Lighting");

  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, m_pointTexID);
  glActiveTexture (GL_TEXTURE1);
  glBindTexture (GL_TEXTURE_2D, m_normalTexID);
  glActiveTexture (GL_TEXTURE2);
  glBindTexture (GL_TEXTURE_2D, m_colourTexID);
  glActiveTexture (GL_TEXTURE3);
  glBindTexture (GL_TEXTURE_2D, m_shadingTexID);
//  glActiveTexture (GL_TEXTURE4);
//  glBindTexture (GL_TEXTURE_2D, m_lightPassTexID);

*/
/*
  // now blit
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, m_pointTexID);
  glActiveTexture (GL_TEXTURE1);
  glBindTexture (GL_TEXTURE_2D, m_normalTexID);
  glActiveTexture (GL_TEXTURE2);
  glBindTexture (GL_TEXTURE_2D, m_colourTexID);
  glActiveTexture (GL_TEXTURE3);
  glBindTexture (GL_TEXTURE_2D, m_shadingTexID);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_lightBuffer);

  glReadBuffer(GL_COLOR_ATTACHMENT0+m_debugMode);
  int w=width()*devicePixelRatio();
  int h=height()*devicePixelRatio();

  glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
*/

  if(m_debug==true)
  {
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, m_pointTexID);
    glActiveTexture (GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_2D, m_normalTexID);
    glActiveTexture (GL_TEXTURE2);
    glBindTexture (GL_TEXTURE_2D, m_colourTexID);
    glActiveTexture (GL_TEXTURE3);
    glBindTexture (GL_TEXTURE_2D, m_shadingTexID);
    glActiveTexture (GL_TEXTURE4);
    glBindTexture (GL_TEXTURE_2D, m_lightPassTexID);

    glGenerateMipmap(GL_TEXTURE_2D);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ngl::ShaderLib *shader = ngl::ShaderLib::instance();

    shader->use("Debug");
    //shader->setUniform("cam",m_cam->getEye().toVec3());
    shader->setUniform("mode",m_debugMode);

    m_screenQuad->draw();

  }

  // calculate and draw FPS
  glBindFramebuffer (GL_FRAMEBUFFER, 0);

  ++m_frames;
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  m_text->setColour(1,1,0);
  QString text=QString("%1 fps using %2 to draw").arg(m_fps).arg(m_debug ? "ScreenQuad" : "glBlitFramebuffer");
  m_text->renderText(10,20,text);

}


void NGLScene::renderLightPass(const std::string &_shader)
{
  // get the VBO instance and draw the built in teapot
 ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
 ngl::ShaderLib *shader = ngl::ShaderLib::instance();
 glDrawBuffer(GL_NONE);
 glEnable(GL_DEPTH_TEST);
 glDisable(GL_CULL_FACE);
 glClear(GL_STENCIL_BUFFER_BIT);
 glStencilFunc(GL_ALWAYS, 0, 0);
 glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
 glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);


 shader->use(_shader);
  for(float z=-5.0; z<5.0; z+=0.5)
  {
    for(float x=-5.0; x<5.0; x+=0.5)
    {
      m_transform.reset();

      m_transform.setPosition(x,sin(x),z);
      ngl::Mat4 MV;
      ngl::Mat4 MVP;
      ngl::Mat4 M;

      M=m_transform.getMatrix()*m_mouseGlobalTX;
      MV=  m_view*M;
      MVP=  m_project=MV;
      ngl::Mat4 globalMV = MV.transpose();
      ngl::Vec4 p(x,sinf(x),z,1.0f);
      p=globalMV*p;
      shader->setUniform("lightPos",p);

      shader->setUniform("MVP",MVP);
      prim->draw("lightSphere");

    }
  }

	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

}

void NGLScene::drawScene(const std::string &_shader)
{
  glViewport(0,0,m_width,m_height);
  // clear the screen and depth buffer
   // Rotation based on the mouse position for our global
   // transform
   ngl::Mat4 rotX;
   ngl::Mat4 rotY;
   // create the rotation matrices
   rotX.rotateX(m_spinXFace);
   rotY.rotateY(m_spinYFace);
   // multiply the rotations
   m_mouseGlobalTX=rotY*rotX;
   // add the translations
   m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
   m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
   m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  ngl::ShaderLib *shader = ngl::ShaderLib::instance();
  shader->use(_shader);
  glPolygonMode(GL_FRONT_AND_BACK,m_wireframe);
  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,0.0f,0.0f);
    loadMatricesToShader();
    prim->draw("teapot");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",0.0f,1.0f,0.0f);

    m_transform.setPosition(-3,0.0,0.0);
    loadMatricesToShader();
    prim->draw("sphere");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,0.0f,1.0f);

    m_transform.setPosition(3,0.0,0.0);
    loadMatricesToShader();
    prim->draw("cylinder");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",0.0f,0.0f,1.0f);

    m_transform.setPosition(0.0f,0.01f,3.0f);
    loadMatricesToShader();
    prim->draw("cube");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,1.0f,0.0f);

    m_transform.setPosition(-3.0f,0.05f,3.0f);
    loadMatricesToShader();
    prim->draw("torus");
  } // and before a pop

  m_transform.reset();
  {

    m_transform.setPosition(3.0f,0.5f,3.0f);
    loadMatricesToShader();
    prim->draw("icosahedron");
  } // and before a pop

  m_transform.reset();
  {
    m_transform.setPosition(0.0,0.0,-3.0f);
    loadMatricesToShader();
    prim->draw("cone");
  } // and before a pop


  m_transform.reset();
  {
    m_transform.setPosition(-3.0f,0.6f,-3.0f);
    loadMatricesToShader();
    prim->draw("tetrahedron");
  } // and before a pop


  m_transform.reset();
  {
    shader->setUniform("Colour",0.0f,0.0f,0.0f);

    m_transform.setPosition(3.0,0.5,-3.0);
    loadMatricesToShader();
    prim->draw("octahedron");
  } // and before a pop


  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,1.0f,1.0f);

    m_transform.setPosition(0.0,0.5,-6.0);
    loadMatricesToShader();
    prim->draw("football");
  } // and before a pop

  m_transform.reset();
  {
    m_transform.setPosition(-3.0,0.5,-6.0);
    m_transform.setRotation(0,180,0);
    loadMatricesToShader();
    prim->draw("disk");
  } // and before a pop


  m_transform.reset();
  {
    m_transform.setPosition(3.0,0.5,-6.0);
    loadMatricesToShader();
    prim->draw("dodecahedron");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,1.0f,0.0f);

    m_transform.setPosition(1.0,0.35f,1.0);
    m_transform.setScale(1.5,1.5,1.5);
    loadMatricesToShader();
    prim->draw("troll");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,0.0f,0.0f);

    m_transform.setPosition(-1.0,-0.5f,1.0);
    m_transform.setScale(0.1f,0.1f,0.1f);
    loadMatricesToShader();
    prim->draw("dragon");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,0.0f,1.0f);

    m_transform.setPosition(-2.5,-0.5,1.0);
    m_transform.setScale(0.1f,0.1f,0.1f);
    loadMatricesToShader();
    prim->draw("buddah");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",1.0f,0.0f,0.0f);

    m_transform.setPosition(2.5,-0.5,1.0);
    m_transform.setScale(0.1,0.1,0.1);
    loadMatricesToShader();
    prim->draw("bunny");
  } // and before a pop

  m_transform.reset();
  {
    shader->setUniform("Colour",0.0f,0.0f,0.8f);

    m_transform.setPosition(0.0,-0.5,0.0);
    loadMatricesToShader();
    prim->draw("plane");
  } // and before a pop


//  for(float z=-5.0; z<5.0; z+=0.5)
//  {
//    for(float x=-5.0; x<5.0; x+=0.5)
//    {
//      m_transform.reset();

//      m_transform.setPosition(x,0,z);
//      loadMatricesToShader();
//      prim->draw("lightSphere");

//    }
//  }


glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    update();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : m_wireframe=GL_LINE; break;
  // turn off wire frame
  case Qt::Key_S : m_wireframe=GL_FILL; break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;

  case Qt::Key_D : m_debug^=true; break;

  case Qt::Key_1 : m_debugMode = 0; break;
  case Qt::Key_2 : m_debugMode = 1; break;
  case Qt::Key_3 : m_debugMode = 2; break;
  case Qt::Key_4 : m_debugMode = 3; break;
  case Qt::Key_5 : m_debugMode = 4; break;
  case Qt::Key_6 : m_debugMode = 5; break;


  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}




void NGLScene::printFrameBufferInfo(GLuint _fbID)
{

	int res;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &res);
	std::cout<<"Max Color Attachments: "<<res<<"\n";

	glBindFramebuffer(GL_FRAMEBUFFER,_fbID);

	GLint buffer;
	int i = 0;
	do
	{
		glGetIntegerv(GL_DRAW_BUFFER0+i, &buffer);

		if (buffer != GL_NONE)
		{

			std::cout<<"Shader Output Location "<<i<<"- color attachment "<<buffer- GL_COLOR_ATTACHMENT0<<"\n";
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, buffer, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &res);
			std::cout<<"\tAttachment Type: "<< (res==GL_TEXTURE? "Texture":"Render Buffer") <<"\n";
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, buffer, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &res);
			std::cout<<"\tAttachment object name: "<<res<<"\n";
		}
		++i;

	} while (buffer != GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
}


void NGLScene::checkFrambufferStatus()
{
	GLenum e = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	switch (e)
	{

		case GL_FRAMEBUFFER_UNDEFINED:
			std::cout<<"FBO Undefined\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			std::cout<<"FBO Incomplete Attachment\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			std::cout<<"FBO Missing Attachment\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
			std::cout<<"FBO Incomplete Draw Buffer\n";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			std::cout<<"FBO Unsupported\n";
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			std::cout<<"FBO OK\n";
			break;
		default:
			std::cout<<"FBO Problem?\n";
	}

}

void NGLScene::timerEvent(QTimerEvent *_event)
{
	if(_event->timerId() == m_fpsTimer)
		{
			if( m_timer.elapsed() > 1000.0)
			{
				m_fps=m_frames;
				m_frames=0;
				m_timer.restart();
			}
		 }
			// re-draw GL
	update();
}
