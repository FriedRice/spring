/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FarTextureHandler.h"

#include "Game/Camera.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Sim/Objects/SolidObject.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/bitops.h"

#include "System/mmgr.h"
#include "string.h"


#define LOG_SECTION_FAR_TEXTURE_HANDLER "FarTextureHandler"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_FAR_TEXTURE_HANDLER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_FAR_TEXTURE_HANDLER


CFarTextureHandler* farTextureHandler = NULL;

const int CFarTextureHandler::iconSizeX = 32;
const int CFarTextureHandler::iconSizeY = 32;
const int CFarTextureHandler::numOrientations = 8;

CFarTextureHandler::CFarTextureHandler()
{
	farTextureID = 0;
	usedFarTextures = 0;

	// ATI supports 16K textures, which might be a bit too much
	// for this purpose,so we limit it to 4K
	const int maxTexSize = std::min(globalRendering->maxTextureSize, 4096);

	texSizeX = maxTexSize;
	texSizeY = std::max(iconSizeY, 4 * numOrientations * iconSizeX * iconSizeY / texSizeX); //! minimum space for 4 icons
	texSizeY = next_power_of_2(texSizeY);

	if (!fbo.IsValid()) {
		LOG_L(L_WARNING, "framebuffer not valid!");
		return;
	}

	glGenTextures(1, &farTextureID);
	glBindTexture(GL_TEXTURE_2D, farTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texSizeX, texSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	fbo.Bind();
	fbo.AttachTexture(farTextureID);
	const bool status = fbo.CheckStatus("FARTEXTURE");
	if (status) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	fbo.Unbind();

	fbo.reloadOnAltTab = true;
}


CFarTextureHandler::~CFarTextureHandler()
{
	glDeleteTextures(1, &farTextureID);
	queuedForRender.clear();
}


/**
 * @brief Returns the (row, column) pair of a FarTexture in the TextureAtlas.
 */
int2 CFarTextureHandler::GetTextureCoordsInt(const int& farTextureNum, const int& orientation)
{
	const int texnum = (farTextureNum * numOrientations) + orientation;

	const int row = texnum / (texSizeX / iconSizeX);
	const int col = texnum - row * (texSizeX / iconSizeX);
	return int2(col, row);
}


/**
 * @brief Returns the TexCoords of a FarTexture in the TextureAtlas.
 */
float2 CFarTextureHandler::GetTextureCoords(const int& farTextureNum, const int& orientation)
{
	float2 texcoords;

	const int texnum = (farTextureNum * numOrientations) + orientation;

	const int row = texnum / (texSizeX / iconSizeX);
	const int col = texnum - row * (texSizeX / iconSizeX);

	texcoords.x = (float(iconSizeX) / texSizeX) * col;
	texcoords.y = (float(iconSizeY) / texSizeY) * row;

	return texcoords;
}


/**
 * @brief Really create the far texture for the given model.
 */
void CFarTextureHandler::CreateFarTexture(const CSolidObject* obj)
{
	const S3DModel* model = obj->model;

	// make space in the std::vectors
	if (obj->team >= (int)cache.size()) {
		cache.resize(obj->team + 1);
	}
	if (model->id >= (int)cache[obj->team].size()) {
		cache[obj->team].resize(model->id + 1, 0);
	}

	cache[obj->team][model->id] = -1;

	//! check if there is enough free space in the atlas, if not try to resize it
	const unsigned int maxSprites = ((texSizeX / iconSizeX) * (texSizeY / iconSizeY) / numOrientations) - 1;

	if (usedFarTextures >= maxSprites) {
		const int oldTexSizeY = texSizeY;

		if (globalRendering->supportNPOTs) {
			texSizeY += std::max(iconSizeY,  4 * numOrientations * iconSizeX * iconSizeY / texSizeX); //! minimum additional space for 4 icons
		} else {
			texSizeY <<= 1;
		}

		if (texSizeY > globalRendering->maxTextureSize) {
			LOG_L(L_DEBUG, "Out of farTextures"); 
			texSizeY = oldTexSizeY;
			return;
		}

		unsigned char* oldPixels = new unsigned char[texSizeX*texSizeY*4];
		glBindTexture(GL_TEXTURE_2D, farTextureID);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, oldPixels);
		memset(oldPixels + texSizeX*oldTexSizeY*4, 0, texSizeX*(texSizeY - oldTexSizeY)*4);

		GLuint newFarTextureID;
		glGenTextures(1, &newFarTextureID);
		glBindTexture(GL_TEXTURE_2D, newFarTextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texSizeX, texSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, oldPixels);
		delete[] oldPixels;

		fbo.Bind();
		fbo.DetachAll();

		glDeleteTextures(1, &farTextureID);
		farTextureID = newFarTextureID;

		fbo.AttachTexture(farTextureID);
		fbo.CheckStatus("FARTEXTURE");
		fbo.Unbind();
	}

	if (!fbo.IsValid()) {
		LOG_L(L_DEBUG, "framebuffer not valid!");
		return;
	}

	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, texSizeX, texSizeY); //! delete it after finished rendering to the texture
	fbo.CheckStatus("FARTEXTURE");

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_BLEND);

	unitDrawer->SetupForUnitDrawing();
	unitDrawer->GetOpaqueModelRenderer(model->type)->PushRenderState();

	if (model->type != MODELTYPE_3DO) {
		// FIXME: for some strange reason we need to invert the culling, why?
		if (model->type == MODELTYPE_S3O) {
			glCullFace(GL_FRONT);
		}
		texturehandlerS3O->SetS3oTexture(model->textureType);
	}

	unitDrawer->SetTeamColour(obj->team);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-model->radius, model->radius, -model->radius, model->radius, -model->radius*1.5f, model->radius*1.5f);
	glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(-1.0f, 1.0f, 1.0f);

	glRotatef(45.0f, 1.0f, 0.0f, 0.0f);

	// light the far-textures from straight above, we do
	// not care much about the actual sun direction here
	static const float4 sunDir = UpVector;

	glLightfv(GL_LIGHT1, GL_POSITION, &sunDir.x);

	// draw the model in 8 different orientations
	for (int orient = 0; orient < numOrientations; ++orient) {
		//! setup viewport
		int2 pos = GetTextureCoordsInt(usedFarTextures, orient);
		glViewport(pos.x * iconSizeX, pos.y * iconSizeY, iconSizeX, iconSizeY);

		glClear(GL_DEPTH_BUFFER_BIT);

		glPushMatrix();
		glTranslatef(0, -model->height * 0.5f, 0);

		//! draw the model to a temporary buffer
		model->DrawStatic();

		glPopMatrix();

		//! rotate by 45 degrees for the next orientation
		glRotatef(-360.0f / numOrientations, 0, 1, 0);
	}

	unitDrawer->GetOpaqueModelRenderer(model->type)->PopRenderState();
	unitDrawer->CleanUpUnitDrawing();

	//glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glPopAttrib();

	fbo.Detach(GL_DEPTH_ATTACHMENT_EXT);
	fbo.Unbind();

	cache[obj->team][model->id] = ++usedFarTextures;
}



void CFarTextureHandler::DrawFarTexture(const CSolidObject* obj, CVertexArray* va) {
	const int farTextureNum = cache[obj->team][obj->model->id];

	//! not found in the atlas
	if (farTextureNum <= 0)
		return;

	const float3 interPos = obj->drawPos + UpVector * obj->model->height * 0.5f;

	//! indicates the orientation to draw
	static const int USHRT_MAX_ = (1 << 16);
	const int orient_step = USHRT_MAX_ / numOrientations;

	int orient = GetHeadingFromVector(-camera->forward.x, -camera->forward.z) - obj->heading;
		orient += USHRT_MAX_;          //! make it positive only
		orient += (orient_step >> 1);  //! we want that frontdir is from -orient_step/2 upto orient_step/2
		orient %= USHRT_MAX_;          //! we have an angle so it's periodical
		orient /= orient_step;         //! get the final direction index

	const float iconSizeX = float(this->iconSizeX) / texSizeX;
	const float iconSizeY = float(this->iconSizeY) / texSizeY;
	const float2 texcoords = GetTextureCoords(farTextureNum - 1, orient);

	const float3 curad = camera->up *    obj->radius;
	const float3 crrad = camera->right * obj->radius;

	va->AddVertexQT(interPos - curad + crrad, texcoords.x, texcoords.y );
	va->AddVertexQT(interPos + curad + crrad, texcoords.x, texcoords.y + iconSizeY);
	va->AddVertexQT(interPos + curad - crrad, texcoords.x + iconSizeX, texcoords.y + iconSizeY);
	va->AddVertexQT(interPos - curad - crrad, texcoords.x + iconSizeX, texcoords.y );
}


void CFarTextureHandler::Queue(const CSolidObject* obj)
{
	queuedForRender.push_back(obj);
}


void CFarTextureHandler::Draw()
{
	if (queuedForRender.empty()) {
		return;
	}

	// create new faricons
	for (GML_VECTOR<const CSolidObject*>::iterator it = queuedForRender.begin(); it != queuedForRender.end(); ++it) {
		const CSolidObject& obj = **it;
		if ((int)cache.size() <= obj.team || (int)cache[obj.team].size() <= obj.model->id || !cache[obj.team][obj.model->id]) {
			CreateFarTexture(*it);
		}
	}

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, farTextureID);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3fv((const GLfloat*) &unitDrawer->camNorm.x);

	ISky::SetupFog();

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(queuedForRender.size() * 4, 0, VA_SIZE_T);
	for (GML_VECTOR<const CSolidObject*>::iterator it = queuedForRender.begin(); it != queuedForRender.end(); ++it) {
		DrawFarTexture(*it, va);
	}

	va->DrawArrayT(GL_QUADS);
	glDisable(GL_ALPHA_TEST);

	queuedForRender.clear();
}
