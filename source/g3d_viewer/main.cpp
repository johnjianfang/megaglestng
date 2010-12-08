#include "main.h"

#include <stdexcept>

#include "model_gl.h"
#include "graphics_interface.h"
#include "util.h"
#include "conversion.h"
#include "platform_common.h"
#include "xml_parser.h"
#include <iostream>
#include <wx/event.h>
#include "game_constants.h"

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Xml;

using namespace std;
using namespace Glest::Game;

#ifdef _WIN32
const char *folderDelimiter = "\\";
#else
const char *folderDelimiter = "/";
#endif

int GameConstants::updateFps= 40;
int GameConstants::cameraFps= 100;

namespace Shared{ namespace G3dViewer{

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str) {
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str) {
	return wxString(str.c_str(), wxConvUTF8);
}

// ===============================================
// 	class MainWindow
// ===============================================

const string g3dviewerVersionString= "v1.3.6";
const string MainWindow::winHeader= "G3D viewer " + g3dviewerVersionString;

MainWindow::MainWindow(const string &modelPath)
    :	wxFrame(NULL, -1, ToUnicode(winHeader),wxPoint(Renderer::windowX, Renderer::windowY),
		wxSize(Renderer::windowW, Renderer::windowH))
{
    //getGlPlatformExtensions();
	renderer= Renderer::getInstance();
	if(modelPath != "") {
		this->modelPathList.push_back(modelPath);
	}
	model= NULL;
	playerColor= Renderer::pcRed;

	speed= 0.025f;

	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER }; // to prevent flicker
	glCanvas = new GlCanvas(this, args);

    //getGlPlatformExtensions();

	//glCanvas->SetCurrent();
	//renderer->init();

	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("&Load G3d model"), wxT("Load 3D model"));
	menuFile->Append(miFileLoadParticleXML, wxT("Load &Particle XML"), wxT("Press ctrl before menu for keeping current particles"));
	menuFile->Append(miFileLoadProjectileParticleXML, wxT("Load P&rojectile Particle XML"), wxT("Press ctrl before menu for keeping current projectile particles"));
	menuFile->Append(miFileLoadSplashParticleXML, wxT("Load &Splash Particle XML"), wxT("Press ctrl before menu for keeping current splash particles"));
	menuFile->Append(miFileClearAll, wxT("&Clear All"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menu->Append(menuFile, wxT("&File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("&Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("&Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("&Grid"));
	menu->Append(menuMode, wxT("&Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("&Slower\t-"));
	menuSpeed->Append(miSpeedFaster, wxT("&Faster\t+"));
	menuSpeed->AppendSeparator();
	menuSpeed->Append(miRestart, wxT("&Restart particles\tR"), wxT("Restart particle animations, this also reloads model and particle files if they are changed"));
	menu->Append(menuSpeed, wxT("&Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->AppendCheckItem(miColorRed, wxT("&Red\t0"));
	menuCustomColor->AppendCheckItem(miColorBlue, wxT("&Blue\t1"));
	menuCustomColor->AppendCheckItem(miColorGreen, wxT("&Green\t2"));
	menuCustomColor->AppendCheckItem(miColorYellow, wxT("&Yellow\t3"));
	menuCustomColor->AppendCheckItem(miColorWhite, wxT("&White\t4"));
	menuCustomColor->AppendCheckItem(miColorCyan, wxT("&Cyan\t5"));
	menuCustomColor->AppendCheckItem(miColorOrange, wxT("&Orange\t6"));
	menuCustomColor->AppendCheckItem(miColorMagenta, wxT("&Pink\t7")); // it is called Pink everywhere else so...
	menu->Append(menuCustomColor, wxT("&Custom Color"));

	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorRed, true);

	SetMenuBar(menu);

	//misc
	model= NULL;
	rotX= 0.0f;
	rotY= 0.0f;
	zoom= 1.0f;
	backBrightness= 0.3f;
	gridBrightness= 1.0f;
	lightBrightness= 0.3f;
	lastX= 0;
	lastY= 0;
	anim= 0.0f;

	CreateStatusBar();

	wxInitAllImageHandlers();
#ifdef WIN32

#if defined(__MINGW32__)
	wxIcon icon(ToUnicode("IDI_ICON1"));
#else
    wxIcon icon("IDI_ICON1");
#endif

#else
	wxIcon icon;
	std::ifstream testFile("g3dviewer.ico");
	if(testFile.good())	{
		testFile.close();
		icon.LoadFile(wxT("g3dviewer.ico"),wxBITMAP_TYPE_ICO);
	}
#endif
	SetIcon(icon);

	timer = new wxTimer(this);
	timer->Start(100);

	glCanvas->SetFocus();
}

MainWindow::~MainWindow(){
	delete renderer;
	delete model;
	delete timer;
	delete glCanvas;

}

void MainWindow::init(){
	glCanvas->SetCurrent();
	renderer->init();

	loadModel("");
}

void MainWindow::onPaint(wxPaintEvent &event){
	renderer->reset(GetClientSize().x, GetClientSize().y, playerColor);

	renderer->transform(rotX, rotY, zoom);
	renderer->renderGrid();

	renderer->renderTheModel(model, anim);

	//int updateLoops = 100;
	int updateLoops = 1;
	for(int i=0; i< updateLoops; ++i) {
		renderer->updateParticleManager();
	}

	renderer->renderParticleManager();

	glCanvas->SwapBuffers();
}

void MainWindow::onClose(wxCloseEvent &event){
	// release memory first (from onMenuFileClearAll)

	modelPathList.clear();
	particlePathList.clear();
	particleProjectilePathList.clear();
	particleSplashPathList.clear(); // as above

	timer->Stop();
	renderer->end();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();

	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();
	splashParticleSystems.clear(); // as above
	splashParticleSystemTypes.clear();

	delete model;
	model = NULL;

	delete this;
}

// for the mousewheel
void MainWindow::onMouseWheelDown(wxMouseEvent &event) {
	wxPaintEvent paintEvent;
	zoom*= 1.1f;
	zoom= clamp(zoom, 0.1f, 10.0f);
	onPaint(paintEvent);
}

void MainWindow::onMouseWheelUp(wxMouseEvent &event) {
	wxPaintEvent paintEvent;
	zoom*= 0.90909f;
	zoom= clamp(zoom, 0.1f, 10.0f);
	onPaint(paintEvent);
}


void MainWindow::onMouseMove(wxMouseEvent &event){
	int x= event.GetX();
	int y= event.GetY();
	wxPaintEvent paintEvent;

	if(event.LeftIsDown()){
		rotX+= clamp(lastX-x, -10, 10);
		rotY+= clamp(lastY-y, -10, 10);
		onPaint(paintEvent);
	}
	else if(event.RightIsDown()){
		zoom*= 1.0f+(lastX-x+lastY-y)/100.0f;
		zoom= clamp(zoom, 0.1f, 10.0f);
		onPaint(paintEvent);
	}

	lastX= x;
	lastY= y;
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("G3D files (*.g3d)|*.g3d"));

	fileDialog.SetMessage(wxT("Selecting Glest Model for current view."));

	if(fileDialog.ShowModal()==wxID_OK){
		modelPathList.clear();
		loadModel((const char*)wxFNCONV(fileDialog.GetPath().c_str()));
	}
	isControlKeyPressed = false;
}

void MainWindow::onMenuFileLoadParticleXML(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("XML files (*.xml)|*.xml"));

	if(isControlKeyPressed == true) {
		fileDialog.SetMessage(wxT("Adding Mega-Glest particle to current view."));
	}
	else {
		fileDialog.SetMessage(wxT("Selecting Mega-Glest particle for current view."));
	}

	if(fileDialog.ShowModal()==wxID_OK){
		string path = (const char*)wxFNCONV(fileDialog.GetPath().c_str());
		loadParticle(path);
	}
	isControlKeyPressed = false;
}

void MainWindow::onMenuFileLoadProjectileParticleXML(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("XML files (*.xml)|*.xml"));

	if(isControlKeyPressed == true) {
		fileDialog.SetMessage(wxT("Adding Mega-Glest projectile particle to current view."));
	}
	else {
		fileDialog.SetMessage(wxT("Selecting Mega-Glest projectile particle for current view."));
	}

	if(fileDialog.ShowModal()==wxID_OK){
		string path = (const char*)wxFNCONV(fileDialog.GetPath().c_str());
		loadProjectileParticle(path);
	}
	isControlKeyPressed = false;
}

void MainWindow::onMenuFileLoadSplashParticleXML(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("XML files (*.xml)|*.xml"));

	if(isControlKeyPressed == true) {
		fileDialog.SetMessage(wxT("Adding Mega-Glest splash particle to current view."));
	}
	else {
		fileDialog.SetMessage(wxT("Selecting Mega-Glest splash particle for current view."));
	}

	if(fileDialog.ShowModal()==wxID_OK){
		string path = (const char*)wxFNCONV(fileDialog.GetPath().c_str());
		loadSplashParticle(path);
	}
	isControlKeyPressed = false;
}  // is it possible to join loadParticle(), loadProjectileParticle() and loadSplashParticle() to one method?


void MainWindow::onMenuFileClearAll(wxCommandEvent &event){
	modelPathList.clear();
	particlePathList.clear();
	particleProjectilePathList.clear();
	particleSplashPathList.clear(); // as above

	timer->Stop();
	renderer->end();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();

	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();
	splashParticleSystems.clear(); // as above
	splashParticleSystemTypes.clear();

	delete model;
	model = NULL;

	loadModel("");
	loadParticle("");
	loadProjectileParticle("");
	loadSplashParticle(""); // as above

	GetStatusBar()->SetStatusText(ToUnicode(""));
	timer->Start(100);
	isControlKeyPressed = false;
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::loadModel(string path) {
	if(path != "" && fileExists(path) == true) {
		this->modelPathList.push_back(path);
	}

    string titlestring=winHeader;
	for(unsigned int idx =0; idx < this->modelPathList.size(); idx++) {
		string modelPath = this->modelPathList[idx];

		timer->Stop();
		delete model;
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, modelPath);
		model= tmpModel;
		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo().c_str()));
		timer->Start(100);
		titlestring =  extractFileFromDirectoryPath(modelPath) + " - "+ titlestring;
	}
	SetTitle(ToUnicode(titlestring));
}

void MainWindow::loadParticle(string path) {
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particlePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particlePathList.clear();
			this->particlePathList.push_back(path);
		}
	}

	try{
	if(this->particlePathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particlePathList.size(); idx++) {
			string particlePath = this->particlePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = -1;
			int height = -1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML);
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();
			unitParticleSystemType->load(dir,  dir + folderDelimiter + particlePath, renderer);
			unitParticleSystemTypes.push_back(unitParticleSystemType);

			for(std::vector<UnitParticleSystemType *>::const_iterator it= unitParticleSystemTypes.begin(); it != unitParticleSystemTypes.end(); ++it) {
				UnitParticleSystem *ups= new UnitParticleSystem(200);
				(*it)->setValues(ups);
				if(size > 0) {
					//getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					ups->setPos(vec);
				}
				//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
				ups->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));
				unitParticleSystems.push_back(ups);
				renderer->manageParticleSystem(ups);

				ups->setVisible(true);
			}

			if(path != "" && fileExists(path) == true) {
				renderer->initModelManager();
				renderer->initTextureManager();
			}
		}
		SetTitle(ToUnicode(titlestring));
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageBox( ToUnicode(e.what()), wxT("Not a Mega-Glest particle XML file, or broken"), wxICON_ERROR);
	}
	timer->Start(100);
}

void MainWindow::loadProjectileParticle(string path) {
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particleProjectilePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleProjectilePathList.clear();
			this->particleProjectilePathList.push_back(path);
		}
	}

	try {
	if(this->particleProjectilePathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleProjectilePathList.size(); idx++) {
			string particlePath = this->particleProjectilePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = -1;
			int height = -1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML);
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			XmlTree xmlTree;
			xmlTree.load(dir + folderDelimiter + particlePath);
			const XmlNode *particleSystemNode= xmlTree.getRootNode();

			// std::cout << "Loaded successfully, loading values..." << std::endl;

			ParticleSystemTypeProjectile *projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(dir,  dir + folderDelimiter + particlePath,renderer);

			// std::cout << "Values loaded, about to read..." << std::endl;

			projectileParticleSystemTypes.push_back(projectileParticleSystemType);

			for(std::vector<ParticleSystemTypeProjectile *>::const_iterator it= projectileParticleSystemTypes.begin(); it != projectileParticleSystemTypes.end(); ++it) {

				ProjectileParticleSystem *ps = (*it)->create();

				if(size > 0) {
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);
					ps->setPath(vec, vec2);
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				projectileParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);

				//MessageBox(NULL,"hi","hi",MB_OK);
				//psProj= pstProj->create();
				//psProj->setPath(startPos, endPos);
				//psProj->setObserver(new ParticleDamager(unit, this, gameCamera));
				//psProj->setVisible(visible);
				//psProj->setFactionColor(unit->getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
				//renderer.manageParticleSystem(psProj, rsGame);
			}
		}
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
			renderer->initTextureManager();
		}
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageBox( ToUnicode(e.what()), wxT("Not a Mega-Glest projectile particle XML file, or broken"), wxICON_ERROR);
	}
	timer->Start(100);
}

void MainWindow::loadSplashParticle(string path) {  // uses ParticleSystemTypeSplash::load  (and own list...)
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		splashParticleSystems.clear();
		splashParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particleSplashPathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleSplashPathList.clear();
			this->particleSplashPathList.push_back(path);
		}
	}

	try {
	if(this->particleSplashPathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleSplashPathList.size(); idx++) {
			string particlePath = this->particleSplashPathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = -1;
			int height = -1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML);
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			XmlTree xmlTree;
			xmlTree.load(dir + folderDelimiter + particlePath);
			const XmlNode *particleSystemNode= xmlTree.getRootNode();

			// std::cout << "Loaded successfully, loading values..." << std::endl;

			ParticleSystemTypeSplash *splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(dir,  dir + folderDelimiter + particlePath,renderer); // <---- only that must be splash...

			// std::cout << "Values loaded, about to read..." << std::endl;

			splashParticleSystemTypes.push_back(splashParticleSystemType);

                                      //ParticleSystemTypeSplash
			for(std::vector<ParticleSystemTypeSplash *>::const_iterator it= splashParticleSystemTypes.begin(); it != splashParticleSystemTypes.end(); ++it) {

				SplashParticleSystem *ps = (*it)->create();

				if(size > 0) {
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					//Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);   // <------- removed relative projectile
					//ps->setPath(vec, vec2);                                       // <------- removed relative projectile
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				splashParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);
			}
		}
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
			renderer->initTextureManager();
		}
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageBox( ToUnicode(e.what()), wxT("Not a Mega-Glest splash particle XML file, or broken"), wxICON_ERROR);
	}
	timer->Start(100);
}

void MainWindow::onMenuModeNormals(wxCommandEvent &event){
	renderer->toggleNormals();
	menuMode->Check(miModeNormals, renderer->getNormals());
}

void MainWindow::onMenuModeWireframe(wxCommandEvent &event){
	renderer->toggleWireframe();
	menuMode->Check(miModeWireframe, renderer->getWireframe());
}

void MainWindow::onMenuModeGrid(wxCommandEvent &event){
	renderer->toggleGrid();
	menuMode->Check(miModeGrid, renderer->getGrid());
}

void MainWindow::onMenuSpeedSlower(wxCommandEvent &event){
	speed/= 1.5f;
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	speed*= 1.5f;
}

// set menu checkboxes to what player color is used
void MainWindow::onMenuColorRed(wxCommandEvent &event){
	playerColor= Renderer::pcRed;
	menuCustomColor->Check(miColorRed, true);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorBlue(wxCommandEvent &event){
	playerColor= Renderer::pcBlue;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, true);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorGreen(wxCommandEvent &event){
	playerColor= Renderer::pcGreen;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, true);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorYellow(wxCommandEvent &event){
	playerColor= Renderer::pcYellow;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, true);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorWhite(wxCommandEvent &event){
	playerColor= Renderer::pcWhite;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, true);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorCyan(wxCommandEvent &event){
	playerColor= Renderer::pcCyan;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, true);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorOrange(wxCommandEvent &event){
	playerColor= Renderer::pcOrange;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, true);
	menuCustomColor->Check(miColorMagenta, false);
}

void MainWindow::onMenuColorMagenta(wxCommandEvent &event){
	playerColor= Renderer::pcMagenta;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorGreen, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorWhite, false);
	menuCustomColor->Check(miColorCyan, false);
	menuCustomColor->Check(miColorOrange, false);
	menuCustomColor->Check(miColorMagenta, true);
}


void MainWindow::onTimer(wxTimerEvent &event){
	wxPaintEvent paintEvent;

	anim= anim+speed;
	if(anim>1.0f){
		anim-= 1.f;
	}
	onPaint(paintEvent);
}

string MainWindow::getModelInfo(){
	string str;

	if(model!=NULL){
		str+= "Meshes: "+intToStr(model->getMeshCount());
		str+= ", Vertices: "+intToStr(model->getVertexCount());
		str+= ", Triangles: "+intToStr(model->getTriangleCount());
		str+= ", Version: "+intToStr(model->getFileVersion());
	}

	return str;
}

void MainWindow::onKeyDown(wxKeyEvent &e) {

	// std::cout << "e.ControlDown() = " << e.ControlDown() << " e.GetKeyCode() = " << e.GetKeyCode() << " isCtrl = " << (e.GetKeyCode() == WXK_CONTROL) << std::endl;

    // Note: This ctrl-key handling is buggy since it never resests when ctrl is released later, so I reset it at end of loadcommands for now.
	if(e.ControlDown() == true || e.GetKeyCode() == WXK_CONTROL) {
		isControlKeyPressed = true;
	}
	else {
		isControlKeyPressed = false;
	}

	// std::cout << "isControlKeyPressed = " << isControlKeyPressed << std::endl;


	// here also because + and - hotkeys don't work for numpad automaticly
	if (e.GetKeyCode() == 388) speed*= 1.5f; //numpad+
	else if (e.GetKeyCode() == 390) speed/= 1.5f; //numpad-
	else if (e.GetKeyCode() == 87) {
	    glClearColor(0.6f, 0.6f, 0.6f, 1.0f); //w  key //backgroundcolor constant 0.3 -> 0.6

	}

	// some posibility to adjust brightness:
    /*
    else if (e.GetKeyCode() == 322) { // Ins  - Grid
		gridBrightness += 0.1f; if (gridBrightness >1.0) gridBrightness =1.0;
	}
	else if (e.GetKeyCode() == 127) { // Del
		gridBrightness -= 0.1f; if (gridBrightness <0) gridBrightness =0;
	}
	*/
	else if (e.GetKeyCode() == 313) { // Home  - Background
		backBrightness += 0.1f; if (backBrightness >1.0) backBrightness=1.0;
		glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
	}
	else if (e.GetKeyCode() == 312) { // End
		backBrightness -= 0.1f; if (backBrightness<0) backBrightness=0;
		glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
	}
	else if (e.GetKeyCode() == 366) { // PgUp  - Lightning of model
		lightBrightness += 0.1f; if (lightBrightness >1.0) lightBrightness =1.0;
		Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
		glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
	}
	else if (e.GetKeyCode() == 367) { // pgDn
		lightBrightness -= 0.1f; if (lightBrightness <0) lightBrightness =0;
		Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
		glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
	}
}

void MainWindow::onMenuRestart(wxCommandEvent &event){
	// std::cout << "pressed R (restart particle animation)" << std::endl;
	renderer->end();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();
	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();
	splashParticleSystems.clear(); // as above
	splashParticleSystemTypes.clear();

	loadModel("");
	loadParticle("");
	loadProjectileParticle("");
	loadSplashParticle(""); // as above

	renderer->initModelManager();
	renderer->initTextureManager();
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(miFileLoadParticleXML, MainWindow::onMenuFileLoadParticleXML)
	EVT_MENU(miFileLoadProjectileParticleXML, MainWindow::onMenuFileLoadProjectileParticleXML)
	EVT_MENU(miFileLoadSplashParticleXML, MainWindow::onMenuFileLoadSplashParticleXML)
	EVT_MENU(miFileClearAll, MainWindow::onMenuFileClearAll)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)
	EVT_MENU(miRestart, MainWindow::onMenuRestart)

	EVT_MENU(miColorRed, MainWindow::onMenuColorRed)
	EVT_MENU(miColorBlue, MainWindow::onMenuColorBlue)
	EVT_MENU(miColorGreen, MainWindow::onMenuColorGreen)
	EVT_MENU(miColorYellow, MainWindow::onMenuColorYellow)
	EVT_MENU(miColorWhite, MainWindow::onMenuColorWhite)
	EVT_MENU(miColorCyan, MainWindow::onMenuColorCyan)
	EVT_MENU(miColorOrange, MainWindow::onMenuColorOrange)
	EVT_MENU(miColorMagenta, MainWindow::onMenuColorMagenta)
END_EVENT_TABLE()

// =====================================================
//	class GlCanvas
// =====================================================

void translateCoords(wxWindow *wnd, int &x, int &y) {
#ifdef WIN32
	int cx, cy;
	wnd->GetPosition(&cx, &cy);
	x += cx;
	y += cy;
#endif
}

GlCanvas::GlCanvas(MainWindow *	mainWindow):
    wxGLCanvas(mainWindow, -1, wxDefaultPosition)
{
    this->mainWindow = mainWindow;
}

// to prevent flicker
GlCanvas::GlCanvas(MainWindow *	mainWindow, int *args)
		: wxGLCanvas(mainWindow, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->mainWindow = mainWindow;
}

// for the mousewheel
void GlCanvas::onMouseWheel(wxMouseEvent &event) {
	if(event.GetWheelRotation()>0) mainWindow->onMouseWheelDown(event);
	else mainWindow->onMouseWheelUp(event);
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
    mainWindow->onMouseMove(event);
}

void GlCanvas::onKeyDown(wxKeyEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onKeyDown(event);
}

//    EVT_SPIN_DOWN(GlCanvas::onMouseDown)
//    EVT_SPIN_UP(GlCanvas::onMouseDown)
//    EVT_MIDDLE_DOWN(GlCanvas::onMouseWheel)
//    EVT_MIDDLE_UP(GlCanvas::onMouseWheel)

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
    EVT_MOUSEWHEEL(GlCanvas::onMouseWheel)
    EVT_MOTION(GlCanvas::onMouseMove)
    EVT_KEY_DOWN(GlCanvas::onKeyDown)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	std::string modelPath;
	if(argc==2){
		if(argv[1][0]=='-') {   // any flag gives help and exits program.
			std::cout << "G3D viewer " << g3dviewerVersionString << std::endl << std::endl;
			std::cout << "glest_g3dviewer [G3D 3D-MODEL FILE]" << std::endl << std::endl;
			std::cout << "Displays glest 3D-models and unit/projectile/splash particle systems."  << std::endl;
			std::cout << "rotate with left mouse button, zoom with right mouse button or mousewheel."  << std::endl;
			std::cout << "Use ctrl to load more than one particle system." << std::endl;
			std::cout << "Press R to restart particles, this also reloads all files if they are changed."  << std::endl << std::endl;
			exit (0);
		}

#if defined(__MINGW32__)
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(argv[1]));
		modelPath = tmp_buf;
#else
        modelPath = wxFNCONV(argv[1]);
#endif

	}

	mainWindow= new MainWindow(modelPath);
	mainWindow->Show();
	mainWindow->init();

	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
}

int App::OnExit(){
	return 0;
}

}}//end namespace

IMPLEMENT_APP(Shared::G3dViewer::App)
