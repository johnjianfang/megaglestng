#include "renderer.h"

#include "graphics_factory_gl.h"
#include "graphics_interface.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Shared{ namespace G3dViewer{

// ===============================================
// 	class MeshCallbackTeamColor
// ===============================================

void MeshCallbackTeamColor::execute(const Mesh *mesh){

	//team color
	if(mesh->getCustomTexture() && teamTexture!=NULL){
		//texture 0
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		//set color to interpolation
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		//texture 1
		glActiveTexture(GL_TEXTURE1);
		glMultiTexCoord2f(GL_TEXTURE1, 0.f, 0.f);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(teamTexture)->getHandle());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE0);
	}
	else{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

// ===============================================
// 	class Renderer
// ===============================================

Renderer::Renderer(){
	normals= false;
	wireframe= false;
	grid= true;
	modelRenderer = NULL;
	textureManager = NULL;
	particleRenderer = NULL;
	modelManager = NULL;
}

Renderer::~Renderer(){
	delete modelRenderer;
	delete textureManager;
	delete particleRenderer;

	//resources
	delete particleManager;
	delete modelManager;
}

Renderer * Renderer::getInstance(){
	static Renderer * renderer = new Renderer();
	return renderer;
}

void Renderer::transform(float rotX, float rotY, float zoom){
	assertGl();

	glMatrixMode(GL_MODELVIEW);
	glRotatef(rotY, 1.0f, 0.0f, 0.0f);
	glRotatef(rotX, 0.0f, 1.0f, 0.0f);
	glScalef(zoom, zoom, zoom);
	Vec4f pos(-8.0f, 5.0f, 10.0f, 0.0f);
	glLightfv(GL_LIGHT0,GL_POSITION, pos.ptr());

	assertGl();
}

void Renderer::checkGlCaps(){

	//opengl 1.3
	if(!isGlVersionSupported(1, 3, 0)){

		string message;

		message += "Your system supports OpenGL version \"";
 		message += getGlVersion() + string("\"\n");
 		message += "Glest needs at least version 1.3 to work\n";
 		message += "You may solve this problem by installing your latest video card drivers";

 		throw runtime_error(message.c_str());
	}

	//opengl 1.4 or extension
	if(!isGlVersionSupported(1, 4, 0)){
		checkExtension("GL_ARB_texture_env_crossbar", "Glest");
	}
}

void Renderer::checkExtension(const string &extension, const string &msg){
	if(!isGlExtensionSupported(extension.c_str())){
		string str= "OpenGL extension not supported: " + extension +  ", required for " + msg;
		throw runtime_error(str);
	}
}

Texture2D * Renderer::getNewTexture2D() {
	Texture2D *newTexture = textureManager->newTexture2D();
	return newTexture;
}

Model * Renderer::getNewModel() {
	Model *newModel = modelManager->newModel();
	return newModel;
}

void Renderer::init(){
	assertGl();

    //!!!glInitNames();
    //!!!checkGlCaps();

	GraphicsFactory *gf= new GraphicsFactoryGl();
	GraphicsInterface::getInstance().setFactory(gf);

	modelRenderer= gf->newModelRenderer();
	textureManager= gf->newTextureManager();
	particleRenderer= gf->newParticleRenderer();

	//resources
	particleManager= gf->newParticleManager();

	modelManager = gf->newModelManager();
	modelManager->setTextureManager(textureManager);

	//red tex
	customTextureRed= textureManager->newTexture2D();
	customTextureRed->getPixmap()->init(1, 1, 3);
	customTextureRed->getPixmap()->setPixel(0, 0, Vec3f(1.f, 0.f, 0.f));

	//blue tex
	customTextureBlue= textureManager->newTexture2D();
	customTextureBlue->getPixmap()->init(1, 1, 3);
	customTextureBlue->getPixmap()->setPixel(0, 0, Vec3f(0.f, 0.f, 1.f));

	//yellow tex
	customTextureYellow= textureManager->newTexture2D();
	customTextureYellow->getPixmap()->init(1, 1, 3);
	customTextureYellow->getPixmap()->setPixel(0, 0, Vec3f(1.f, 1.f, 0.f));

	//green
	customTextureGreen= textureManager->newTexture2D();
	customTextureGreen->getPixmap()->init(1, 1, 3);
	customTextureGreen->getPixmap()->setPixel(0, 0, Vec3f(0.f, 0.5f, 0.f));

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	Vec4f diffuse= Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
	Vec4f ambient= Vec4f(0.3f, 0.3f, 0.3f, 1.0f);
	Vec4f specular= Vec4f(0.1f, 0.1f, 0.1f, 1.0f);

	glLightfv(GL_LIGHT0,GL_AMBIENT, ambient.ptr());
	glLightfv(GL_LIGHT0,GL_DIFFUSE, diffuse.ptr());
	glLightfv(GL_LIGHT0,GL_SPECULAR, specular.ptr());

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	assertGl();
}

void Renderer::reset(int w, int h, PlayerColor playerColor){
	assertGl();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, static_cast<float>(w)/h, 1.0f, 200.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, -1.5, -5);

	Texture2D *customTexture;
	switch(playerColor){
	case pcRed:
		customTexture= customTextureRed;
		break;
	case pcBlue:
		customTexture= customTextureBlue;
		break;
	case pcYellow:
		customTexture= customTextureYellow;
		break;
	case pcGreen:
		customTexture= customTextureGreen;
		break;
	default:
		assert(false);
	}
	meshCallbackTeamColor.setTeamTexture(customTexture);

	if(wireframe){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}
	else{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	assertGl();
}

void Renderer::renderGrid(){
	if(grid){

		float i;

		assertGl();

		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f);
		for(i=-10.0f; i<=10.0f; i+=1.0f){
			glVertex3f(i, 0.0f, 10.0f);
			glVertex3f(i, 0.0f, -10.0f);
		}
		for(i=-10.0f; i<=10.0f; i+=1.0f){
			glVertex3f(10.f, 0.0f, i);
			glVertex3f(-10.f, 0.0f, i);
		}
		glEnd();

		glPopAttrib();

		assertGl();
	}
}

void Renderer::toggleNormals(){
	normals= normals? false: true;
}

void Renderer::toggleWireframe(){
	wireframe= wireframe? false: true;
}

void Renderer::toggleGrid(){
	grid= grid? false: true;
}

void Renderer::loadTheModel(Model *model, string file){
	model->setTextureManager(textureManager);
	model->loadG3d(file);
	textureManager->init();
	modelManager->init();
}

void Renderer::renderTheModel(Model *model, float f){
	if(model != NULL){
		modelRenderer->begin(true, true, !wireframe, &meshCallbackTeamColor);
		model->updateInterpolationData(f, true);
		modelRenderer->render(model);

		if(normals){
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);
			modelRenderer->renderNormalsOnly(model);
			glPopAttrib();
		}

		modelRenderer->end();
	}
}

void Renderer::manageParticleSystem(ParticleSystem *particleSystem){
	particleManager->manage(particleSystem);
}

void Renderer::updateParticleManager(){
	particleManager->update();
}

void Renderer::renderParticleManager(){
	glPushAttrib(GL_DEPTH_BUFFER_BIT  | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	particleRenderer->renderManager(particleManager, modelRenderer);
	glPopAttrib();
}

Texture2D * Renderer::getPlayerColorTexture(PlayerColor playerColor) {
	Texture2D *customTexture;
	switch(playerColor){
	case pcRed:
		customTexture= customTextureRed;
		break;
	case pcBlue:
		customTexture= customTextureBlue;
		break;
	case pcYellow:
		customTexture= customTextureYellow;
		break;
	case pcGreen:
		customTexture= customTextureGreen;
		break;
	default:
		assert(false);
	}

	return customTexture;
}

void Renderer::initTextureManager() {
	textureManager->init();
}

void Renderer::initModelManager() {
	modelManager->init();
}

void Renderer::end() {
	//delete resources
	//textureManager->end();
	particleManager->end();
	modelManager->end();
}

}}//end namespace
