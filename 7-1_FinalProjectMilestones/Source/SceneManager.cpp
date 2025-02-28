///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// Creates textures for shapes from imported files
	bReturn = CreateGLTexture(
		"textures/wood-floor.jpg",
		"Floor");

	bReturn = CreateGLTexture(
		"textures/Mustard_Yellow_Rubber.jpg",
		"Yellow_rubber");

	bReturn = CreateGLTexture(
		"textures/Martinellis_Label.jpg",
		"Bottle_label");

	bReturn = CreateGLTexture(
		"textures/Aluminium_Foil.jpg",
		"Metal_wrap");

	bReturn = CreateGLTexture(
		"textures/Cup_Blue_Pastel.jpg",
		"Blue_pastel");

	bReturn = CreateGLTexture(
		"textures/Metal_Ribbon.jpg",
		"Metal_Ribbon");

	bReturn = CreateGLTexture(
		"textures/Door.png",
		"Door");

	bReturn = CreateGLTexture(
		"textures/Wood_trim_1.jpg",
		"Trim1");

	bReturn = CreateGLTexture(
		"textures/Wood_trim_2.jpg",
		"Trim2");

	bReturn = CreateGLTexture(
		"textures/Wall_texture.jpg",
		"Wall");

	bReturn = CreateGLTexture(
		"textures/Jam_lable.png",
		"Jam");

	bReturn = CreateGLTexture(
		"textures/Jam_lable_2.png",
		"Jam_2");

	bReturn = CreateGLTexture(
		"textures/Nutrition_Label.png",
		"Nutrition_facts");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Material for pacifier
	OBJECT_MATERIAL pacifierMaterial;
	pacifierMaterial.diffuseColor = glm::vec3(1.0f, 0.85f, 0.2f);
	pacifierMaterial.specularColor = glm::vec3(0.8f, 0.7f, 0.3f);
	pacifierMaterial.shininess = 32.0f;
	pacifierMaterial.tag = "pacifier_material";
	m_objectMaterials.push_back(pacifierMaterial);

	// Material for the floor
	OBJECT_MATERIAL floorMaterial;
	floorMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	floorMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	floorMaterial.shininess = 2.0f;
	floorMaterial.tag = "floor_material";
	m_objectMaterials.push_back(floorMaterial);

	// Material for bottel
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.6f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 128.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Material for metal lid
	OBJECT_MATERIAL metalLidMaterial;
	metalLidMaterial.diffuseColor = glm::vec3(0.1f, 0.3f, 0.1f);
	metalLidMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalLidMaterial.shininess = 64.0f;
	metalLidMaterial.tag = "metal_lid_material";
	m_objectMaterials.push_back(metalLidMaterial);

	// Material for light pink plastic bottle
	OBJECT_MATERIAL plasticBottleMaterial;
	plasticBottleMaterial.diffuseColor = glm::vec3(0.95f, 0.85f, 0.9f);
	plasticBottleMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plasticBottleMaterial.shininess = 5.0f;
	plasticBottleMaterial.tag = "plastic_bottle_material";
	m_objectMaterials.push_back(plasticBottleMaterial);

	// Material for glass jar
	OBJECT_MATERIAL brownGlassMaterial;
	brownGlassMaterial.diffuseColor = glm::vec3(0.5f, 0.3f, 0.1f);
	brownGlassMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	brownGlassMaterial.shininess = 128.0f;
	brownGlassMaterial.tag = "brown_glass_material";
	m_objectMaterials.push_back(brownGlassMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting for the scene
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main light: Overhead point light
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 70.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.65f, 0.65f, 0.65f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.075f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.02f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Secondary point light from the front
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 2.0f, 30.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.1f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.03f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the textures for the 3D scene
	LoadSceneTextures();

	// Define the materials that will be used for the objects
	DefineObjectMaterials();

	// Add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadExtraTorusMesh1(0.1f);
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Call the methodes to create each object
	RenderBackground();
	RenderPacifier(2.8f, 0.075f, 2.0f);
	RenderCup(-9.0f, 0.005f, -7.0f);
	RenderBottle(1.0, 0.0, -8.0);
	RenderJam(9.0, 0.001, -4.0);
	RenderBabyLotion(0.0f, 0.0f, 0.0f);
}

/***********************************************************
 *  RenderBackground()
 *
 *  This method is called to render the planes for the
 *  background.
 ***********************************************************/
void SceneManager::RenderBackground()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of bottom plane
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 60.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(1, 1, 1, 1);

	// Set the UV scale for the next texture
	SetTextureUVScale(.3, .3);

	// Set texture to apply to object
	SetShaderTexture("Floor");

	// Set sharder material type
	SetShaderMaterial("floor_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of bottom plane

	// Start of door
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 60.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 64.0f, -50.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, 1);

	// Set texture to apply to object
	SetShaderTexture("Door");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of door

	// Start of door trim
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -50.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, .1);

	// Set texture to apply to object
	SetShaderTexture("Trim2");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of door trim

	// Start of door jam
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 0.50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.5f, -50.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.0, 0.0, 0.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of door jam

	// Start of left wall
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 60.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 64.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, 1);

	// Set texture to apply to object
	SetShaderTexture("Wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of left wall

	// Start of left wall trim
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, .1);

	// Set texture to apply to object
	SetShaderTexture("Trim1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of left wall trim

	// Start of right wall
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 60.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(50.0f, 64.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, 1);

	// Set texture to apply to object
	SetShaderTexture("Wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of right wall

	// Start of right wall trim
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(50.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, .1);

	// Set texture to apply to object
	SetShaderTexture("Trim1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// End of right wall trim
}

/***********************************************************
 *  RenderPacifier()
 *
 *  This method is called to render the shapes for the
 *  pacifier object. Pass XYZ coordinates to move
 *  the whole object at once.
 ***********************************************************/
void SceneManager::RenderPacifier(float Xpos, float Ypos, float Zpos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of Pacifier Handle (Half-Torus)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25f, 1.2f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 225.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.1f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, .2);

	// Set texture to apply to object
	SetShaderTexture("Yellow_rubber");

	// Set shader material type
	SetShaderMaterial("pacifier_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
	/****************************************************************/
	// End of Pacifier Handle (Half-Torus)

	// Start of Pacifier Mouth Piece (Sphere)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 1.7f, 1.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 55.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.15f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(.2, .2);

	// Set texture to apply to object
	SetShaderTexture("Yellow_rubber");

	// Set shader material type
	SetShaderMaterial("pacifier_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// End of Pacifier Mouth Piece (Sphere)

	// Start of Pacifier Nipple Base (Half-Sphere)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.85f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 30.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 315.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.15f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(.2, .2);

	// Set texture to apply to object
	SetShaderTexture("Yellow_rubber");

	// Set shader material type
	SetShaderMaterial("pacifier_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	// End of Pacifier Nipple Base (Half-Sphere)

	// Start of Pacifier Nipple Shaft (Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 1.25f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 30.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 325.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.35f + Xpos, 1.65f + Ypos, 0.35f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(.2, .2);

	// Set texture to apply to object
	SetShaderTexture("Yellow_rubber");

	// Set shader material type
	SetShaderMaterial("pacifier_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Pacifier Nipple Shaft (Cylinder)

	// Start of Pacifier Nipple Tip (Sphere)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.45f, 0.45f, 0.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.9f + Xpos, 2.45f + Ypos, 0.9f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(.2, .2);

	// Set texture to apply to object
	SetShaderTexture("Yellow_rubber");

	// Set shader material type
	SetShaderMaterial("pacifier_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// End of Pacifier Nipple Tip (Sphere)
}

/***********************************************************
 *  RenderBabyLotion()
 *
 *  This method is called to render the shapes for the
 *  baby lotion bottle object. Pass XYZ coordinates to move
 *  the whole object at once.
 ***********************************************************/
void SceneManager::RenderBabyLotion(float Xpos, float Ypos, float Zpos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of Cap (Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.45f, 1.0f, 0.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 6.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.92, 0.87, 0.90, 1.0);

	// Set shader material type
	SetShaderMaterial("plastic_bottle_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, false, true);
	/****************************************************************/
	// End of Cap (Cylinder)

	// Start of Base (Tapered Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 2.0f, 1.40f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 2.001f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.94, 0.67, 0.83, 1.0);

	// Set shader material type
	SetShaderMaterial("plastic_bottle_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
	/****************************************************************/
	// End of Base (Tapered Cylinder)

	// Start of Mid Body (Tapered Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 3.0f, 1.40f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 2.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.94, 0.67, 0.83, 1.0);

	// Set shader material type
	SetShaderMaterial("plastic_bottle_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Mid Body (Tapered Cylinder)
	
	// Start of Upper Body (Tapered Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25f, 1.0f, 0.70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 5.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.94, 0.67, 0.83, .30);

	// Set shader material type
	SetShaderMaterial("plastic_bottle_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
	/****************************************************************/
	// End of Upper Body (Tapered Cylinder)
}

/***********************************************************
 *  RenderCup()
 *
 *  This method is called to render the shapes for the
 *  cup object. Pass XYZ coordinates to move
 *  the whole object at once.
 ***********************************************************/
void SceneManager::RenderCup(float Xpos, float Ypos, float Zpos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of Cup Body (Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.5f, 8.0f, 4.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 0.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, .2);

	// Set texture to apply to object
	SetShaderTexture("Blue_pastel");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, true, true);
	/****************************************************************/
	// End of Cup Body (Cylinder)

	// Start of Cup Handle (Half-Torus)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.75f, 4.0f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 320.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.39f + Xpos, 4.0f + Ypos, -2.94f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.13, 0.25, 0.39, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
	/****************************************************************/
	// End of Cup Cup Handle (Half-Torus)
}

/***********************************************************
 *  RenderJam()
 *
 *  This method is called to render the shapes for the
 *  jar of jam object. Pass XYZ coordinates to move
 *  the whole object at once.
 ***********************************************************/
void SceneManager::RenderJam(float Xpos, float Ypos, float Zpos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of Jam body (Box)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 3.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 2.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(1, 1);  // Set the UV scale for the next texture
	SetShaderTexture("Jam_2");  // Set texture to apply to object
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::right);  // draw the mesh with transformation values


	SetTextureUVScale(1, 1);
	SetShaderTexture("Jam");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::bottom);


	SetTextureUVScale(1, 1);
	SetShaderTexture("Nutrition_facts");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::left);

	
	SetShaderColor(0.28, 0.18, 0.10, 1.0);  // Set the color for the next draw command
	SetShaderMaterial("brown_glass_material");  // Set shader material type
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::front);  // draw the mesh with transformation values
	/****************************************************************/
	// End of Jam body (Box)

	// Start of Jam base (Box)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 0.50f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.28, 0.18, 0.10, 1.0);

	// Set shader material type
	SetShaderMaterial("brown_glass_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	// End of Jam base (Box)

	// Start of Jam upper body (Box)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 1.5f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 4.75f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.28, 0.18, 0.10, 1.0);

	// Set shader material type
	SetShaderMaterial("brown_glass_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	// End of Jam upper body (Box)

	// Start of Jam Neck (Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 1.0f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 5.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.28, 0.18, 0.10, 1.0);

	// Set shader material type
	SetShaderMaterial("brown_glass_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Jam Neck (Cylinder)

	// Start of Jam Lid (Torus + Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 2.5f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 6.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.13, 0.33, 0.19, 1.0);

	// Set shader material type
	SetShaderMaterial("metal_lid_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawExtraTorusMesh1();
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 1.0f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 6.7f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.13, 0.33, 0.19, 1.0);

	// Set shader material type
	SetShaderMaterial("metal_lid_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, true, false);
	/****************************************************************/
	// End of Jam Lid (Torus + Cylinder)
}

/***********************************************************
 *  RenderBottle()
 *
 *  This method is called to render the shapes for the
 *  bottle object. Pass XYZ coordinates to move
 *  the whole object at once.
 ***********************************************************/
void SceneManager::RenderBottle(float Xpos, float Ypos, float Zpos)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Start of Bottle Mid Body (Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.75f, 8.5f, 3.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 100.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(-1, 1);

	// Set texture to apply to object
	SetShaderTexture("Bottle_label");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Bottle Mid Body (Cylinder)

	// Start of Bottle gold ribbon (Tapered-Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.79f, 2.0f, 1.79f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 15.75f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, 1);

	// Set texture to apply to object
	SetShaderTexture("Metal_Ribbon");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Bottle gold ribbon (Tapered-Cylinder)

	// Start of Bottle neck (Cylinder + Torus)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.96f, 5.5f, 0.96f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 17.6f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(1, 1);

	// Set texture to apply to object
	SetShaderTexture("Metal_wrap");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, false, true);

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 22.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the UV scale for the next texture
	SetTextureUVScale(.5, .5);

	// Set texture to apply to object
	SetShaderTexture("Metal_wrap");

	// draw the mesh with transformation values
	m_basicMeshes->DrawExtraTorusMesh1();
	/****************************************************************/
	// End of Bottle neck (Cylinder + Torus)

	// Start of Bottel Bottom (Torus + Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.39f, 3.39f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.16, 0.26, 0.10, .90);

	// Set shader material type
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawExtraTorusMesh1();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.39f, 1.0f, 3.39f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 1.5f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.16, 0.26, 0.10, .90);

	// Set shader material type
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, true, false);
	/****************************************************************/
	// End of Bottel Bottom (Torus + Cylinder)

	// Start of Bottle upper body (Torus + Taperer Cylinder)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.39f, 3.39f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 10.0f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.16, 0.26, 0.10, .90);

	// Set shader material type
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawExtraTorusMesh1();
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.545f, 4.0f, 3.545f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + Xpos, 11.8f + Ypos, 0.0f + Zpos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color for the next draw command
	SetShaderColor(0.16, 0.26, 0.10, .90);

	// Set shader material type
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
	/****************************************************************/
	// End of Bottle upper body (Torus + Taperer Cylinder)
}