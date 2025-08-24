///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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


// New constants for more maintainable code
namespace SceneConstants
{
	// Texture names - makes code more maintainable
	const char* CUP_TEXTURE = "CupTex";
	const char* DRINK_TEXTURE = "DrinkColor";
	const char* COASTER_BASE_TEXTURE = "CoasterTex1";
	const char* COASTER_OVERLAY_TEXTURE = "CoasterTex2";
	const char* TABLE_TEXTURE = "TableTex";
	const char* PLATE_TEXTURE = "PlateTex";
	const char* NAPKIN_TEXTURE = "NapTex";
	const char* BREADBASE_TEXTURE = "BreadBaseTex";
	const char* BREADCRUST_TEXTURE = "BreadCrustTex";
	const char* CHEESE_TEXTURE = "CheeseTex";
	const char* BOTTOMBOOK_TEXTURE = "BottomBookTex";
	const char* MIDDLEBOOK_TEXTURE = "MiddleBookTex";
	const char* TOPBOOK_TEXTURE = "TopBookTex";


	// Material names
	const char* CUP_MATERIAL = "cup";
	const char* DRINK_MATERIAL = "drink";
	const char* TABLE_MATERIAL = "table";
	const char* COASTER_MATERIAL = "coaster";
	const char* PLATE_MATERIAL = "plate";
	const char* NAPKIN_MATERIAL = "napkin";
	const char* BREAD_MATERIAL = "bread";
	const char* BOTTOMBOOK_MATERIAL = "bottombook";
	const char* MIDDLEBOOK_MATERIAL = "middlebook";
	const char* TOPBOOK_MATERIAL = "topbook";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/CoolFabric.jpg", //Cup texture
		"CupTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load cup texture!" << std::endl; // Implement some error handling
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/BlackMarble.jpg", // Drink texture
		"DrinkTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load drink texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/LightWood.jpg", // Coaster wood base
		"CoasterTex1");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load coaster base texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/FlowerOverlay.png", // Coaster floral pattern
		"CoasterTex2");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load coaster overlay texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/WhiteFabric.jpg", // Table cloth grey/white texture
		"TableTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/Napkin.jpg", // Napkin cloth texture
		"NapTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/RedPlaid.jpg", // Book one
		"TopBookTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/RoughBrown.jpg", // Book two
		"MiddleBookTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/BlackLeather.jpg", // Book three
		"BottomBookTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/ShinyRock.jpg", // Plate texture
		"PlateTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/Rough3.jpg", // Bread base texture
		"BreadBaseTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/Rough5.jpg", // Bread crust texture
		"BreadCrustTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load table texture!" << std::endl;
	}

	bReturn = CreateGLTexture(
		"../../../Utilities/textures/YellowInteresting.jpg", // Cheese texture
		"CheeseTex");
	if (!bReturn) {
		std::cout << "ERROR: Failed to load cheese texture!" << std::endl;
	}



	
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
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Define how objects will interact with light: their reflectiveness, brightness, etc.

	// Cup material - wanted to make it light, glossy, and reflective 
	OBJECT_MATERIAL cupMaterial;
	cupMaterial.ambientColor = glm::vec3(0.35f, 0.35f, 0.38f);
	cupMaterial.ambientStrength = 0.4f;
	cupMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.55f);
	cupMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.95f);  // Slightly brighter specular
	cupMaterial.shininess = 64.0f;  // Increased for more ceramic-like shine
	cupMaterial.tag = "cup";
	m_objectMaterials.push_back(cupMaterial);

	// Coffee material - More visible dark liquid
	OBJECT_MATERIAL drinkMaterial;
	drinkMaterial.ambientColor = glm::vec3(0.08f, 0.05f, 0.03f);  // Dark but visible
	drinkMaterial.ambientStrength = 0.5f;                         // Higher ambient for visibility
	drinkMaterial.diffuseColor = glm::vec3(0.15f, 0.10f, 0.07f);  // Coffee brown-ish
	drinkMaterial.specularColor = glm::vec3(0.3f, 0.25f, 0.20f);  // Liquid reflection
	drinkMaterial.shininess = 40.0f;                              // Medium liquid shine
	drinkMaterial.tag = "drink";
	m_objectMaterials.push_back(drinkMaterial);

	// Table material - Gone through many iterations, still not super sold 
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.45f, 0.45f, 0.46f);  // Somewhat darker
	tableMaterial.ambientStrength = 0.25f;                        // Better color-matched
	tableMaterial.diffuseColor = glm::vec3(0.45f, 0.45f, 0.46f);  // Subdued grey
	tableMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.16f); // Very low specular
	tableMaterial.shininess = 2.0f;                               // Matte finish (making it reflective washed out the scene)
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	// Coaster material - simple light-colored wood
	OBJECT_MATERIAL coasterMaterial;
	coasterMaterial.ambientColor = glm::vec3(0.45f, 0.35f, 0.27f);  // A bit lighter warm wood tone
	coasterMaterial.ambientStrength = 0.32f;                        // Slightly more visibility
	coasterMaterial.diffuseColor = glm::vec3(0.6f, 0.47f, 0.35f);   // Lighter rich wood color
	coasterMaterial.specularColor = glm::vec3(0.25f, 0.2f, 0.15f);  // Gentle wood specular
	coasterMaterial.shininess = 16.0f;                              // Natural wood shine
	coasterMaterial.tag = "coaster";
	m_objectMaterials.push_back(coasterMaterial);

	// Bread material - Pretty much matte, wanted the texture to pop
	OBJECT_MATERIAL breadMaterial;
	breadMaterial.ambientColor = glm::vec3(0.4f, 0.32f, 0.22f);   // Bread color
	breadMaterial.ambientStrength = 0.4f;                         // Good visibility
	breadMaterial.diffuseColor = glm::vec3(0.6f, 0.48f, 0.35f);   // Warm bread
	breadMaterial.specularColor = glm::vec3(0.1f, 0.08f, 0.06f);  // Very low specular
	breadMaterial.shininess = 0.5f;                               // Almost matte
	breadMaterial.tag = "bread";
	m_objectMaterials.push_back(breadMaterial);

	// Napkin material - Soft matte fabric, makes it so it stands out against the table
	OBJECT_MATERIAL napkinMaterial;
	napkinMaterial.ambientColor = glm::vec3(0.45f, 0.45f, 0.46f); // Light fabric
	napkinMaterial.ambientStrength = 0.3f;                        // Moderate ambient
	napkinMaterial.diffuseColor = glm::vec3(0.65f, 0.65f, 0.66f); // Clean white-ish
	napkinMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.11f);  // Very low specular
	napkinMaterial.shininess = 0.0f;                              // Fabric matte
	napkinMaterial.tag = "napkin";
	m_objectMaterials.push_back(napkinMaterial);

	// Plate material - Bright, reflective ceramic
	// Has taken the most work by far and still I'm unhappy
	OBJECT_MATERIAL plateMaterial;
	plateMaterial.ambientColor = glm::vec3(0.25f, 0.18f, 0.02f);   // Rich base orange-brown
	plateMaterial.ambientStrength = 0.3f;                            // Low-ish to avoid being washed out
	plateMaterial.diffuseColor = glm::vec3(0.65f, 0.42f, 0.20f);   // Strong diffuse, matches golden ceramic
	plateMaterial.specularColor = glm::vec3(0.8f, 0.75f, 0.70f);    // Bright reflection for gloss
	plateMaterial.shininess = 64.0f;                              // High shininess
	plateMaterial.tag = "plate";
	m_objectMaterials.push_back(plateMaterial);

	// Bottom book material - Black leather-akin book
	OBJECT_MATERIAL bottomBookMaterial;
	bottomBookMaterial.ambientColor = glm::vec3(0.12f, 0.12f, 0.12f);  // Deep black leather
	bottomBookMaterial.ambientStrength = 0.45f;                        // Higher ambient for dark surface
	bottomBookMaterial.diffuseColor = glm::vec3(0.18f, 0.18f, 0.18f);  // Deep black
	bottomBookMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);    // Fairly reflective
	bottomBookMaterial.shininess = 32.0f;                              // Semi-glossy leather shine
	bottomBookMaterial.tag = "bottombook";
	m_objectMaterials.push_back(bottomBookMaterial);

	// Middle book material - Went creative vs source to not have two black books
	OBJECT_MATERIAL middleBookMaterial;
	middleBookMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);    // Medium book base
	middleBookMaterial.ambientStrength = 0.65f;                        // Moderate ambient
	middleBookMaterial.diffuseColor = glm::vec3(0.45f, 0.45f, 0.45f);  // Medium color
	middleBookMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.17f); // Low specular
	middleBookMaterial.shininess = 6.0f;                               // Slight book cover sheen
	middleBookMaterial.tag = "middlebook";
	m_objectMaterials.push_back(middleBookMaterial);

	// Top book material - Red-ish paperback
	OBJECT_MATERIAL topBookMaterial;
	topBookMaterial.ambientColor = glm::vec3(0.5f, 0.2f, 0.15f);       // Reddish book
	topBookMaterial.ambientStrength = 0.3f;                            // Good color visibility
	topBookMaterial.diffuseColor = glm::vec3(0.7f, 0.3f, 0.25f);       // Bright red cover
	topBookMaterial.specularColor = glm::vec3(0.3f, 0.15f, 0.12f);     // Moderate specular for cover
	topBookMaterial.shininess = 2.0f;                                 // Paperback, low shine
	topBookMaterial.tag = "topbook";
	m_objectMaterials.push_back(topBookMaterial);


}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// Define light sources, their position, and their color

	// Changed quite a bit since my last assignment now that I have the full picture
	// Continues to be the hardest part for me to be satisfied with

	 // Primary light - Warm overhead, softer diffuse
	m_pShaderManager->setVec3Value("lightSources[0].position", 2.5f, 7.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.20f, 0.18f, 0.16f);   // Lowered ambient
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.45f, 0.42f, 0.38f);   // Less bright diffuse
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.35f, 0.32f, 0.28f);  // Stronger specular
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 35.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.5f);

	// Rim light - Great for all of the ceramic in my scene
	m_pShaderManager->setVec3Value("lightSources[1].position", 6.0f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.18f, 0.18f, 0.18f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.1f, 1.1f, 1.1f);   // Very strong specular
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 8.0f);

	// Edge light - This was mostly to match the specular light on the books from my source
	m_pShaderManager->setVec3Value("lightSources[2].position", -5.0f, 6.0f, -3.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.03f, 0.03f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.15f, 0.14f, 0.13f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.6f, 0.58f, 0.55f); // Stronger reflection
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 18.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 3.5f);

	// Fill light - Wanted to avoid deep shadows
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 11.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.08f, 0.07f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.15f, 0.14f, 0.13f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 60.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.3f);
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

	LoadSceneTextures(); // Load textures for use

	DefineObjectMaterials(); // Define the materials that will be used for the objects

	SetupSceneLights(); // Add/define the light sources for the scene

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
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
	RenderTable();
	RenderCoaster();
	RenderCoffeeMug();
	RenderPlate();
	RenderNapkin();
	RenderBooks();
	RenderSandwich();
}


// Handles the table object
void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 0.01f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial(SceneConstants::TABLE_MATERIAL); // Use constants
	SetShaderTexture(SceneConstants::TABLE_TEXTURE); // Apply the table texture
	SetTextureUVScale(2.0f, 2.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}

// Handles the coaster object
void SceneManager::RenderCoaster()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Handles pre-object cylinder mesh (coaster)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 0.1f, 1.3f); // Simple flat disk mesh

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.0f, 3.0f); // Match more table-parallel perspective

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f);
	SetShaderTexture(SceneConstants::COASTER_BASE_TEXTURE); // Apply wood base
	SetShaderMaterial(SceneConstants::COASTER_MATERIAL);
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Enable blending for secondary texture overlay

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 0.1f, 1.3f); // Simple flat disk mesh

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.001f, 3.0f); // Match more table-parallel perspective

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::COASTER_OVERLAY_TEXTURE); // Apply flower overlay
	SetShaderMaterial(SceneConstants::COASTER_MATERIAL);
	SetTextureUVScale(3.0f, 3.0f);



	m_basicMeshes->DrawCylinderMesh();
	glDisable(GL_BLEND); // End blending

	/****************************************************************/
}

// Handles the mug  object
void SceneManager::RenderCoffeeMug()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Handles first cylinder mesh (coffee mug body)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.45f, 1.0f); // Match mug size

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.1f, 3.0f); // Worked to better match the built-in camera perspective

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::CUP_TEXTURE);
	SetShaderMaterial(SceneConstants::CUP_MATERIAL); // Apply artistic cup texture (I grew tired of searching for a matching grey)



	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, true, true); // New change. Remove top, better resembles cup
	/****************************************************************/

	// Handles second cylinder mesh (coffee mug "liquid"-- gives the illusion of depth)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.999f, 2.3f, 0.999f); // Emulates either liquid or empty space

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.1f, 3.0f); // Same position as previous cylinders

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 0.5f);
	SetShaderMaterial(SceneConstants::DRINK_MATERIAL);
	SetShaderTexture(SceneConstants::DRINK_TEXTURE);
	SetTextureUVScale(7.0f, 7.0f); // Tiling made the "drink" look more turbulent and believable

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// Handles torus mesh-- coffee mugs handle
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.9f, 0.82f, 0.9f); // Size appropriately

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -55.0f; // Rotate to, again, match more table parallel perspective. Angle was tricky, but I think I got it.
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.6f, 1.3f, 2.3f); // Position so that half-ish of the torus protrudes from the mug.

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial(SceneConstants::CUP_MATERIAL);
	SetShaderTexture(SceneConstants::CUP_TEXTURE);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}


// Handles the plate object
void SceneManager::RenderPlate()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//PLATE
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 0.05f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.0f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::PLATE_TEXTURE);
	SetShaderMaterial(SceneConstants::PLATE_MATERIAL);

	// draw the mesh with transformation values 
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 0.5f, 3.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.45f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::PLATE_TEXTURE);
	SetShaderMaterial(SceneConstants::PLATE_MATERIAL);
	SetTextureUVScale(11.0f, 11.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/******************************************************************/
}


// Handles the napkin object
void SceneManager::RenderNapkin()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.85f, 0.03f, 3.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.10f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 1.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::NAPKIN_TEXTURE);
	SetShaderMaterial(SceneConstants::NAPKIN_MATERIAL);
	SetTextureUVScale(0.9f, 0.9f);

	// draw the mesh with transformation values 
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 0.03f, 3.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -13.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.65f, 0.34f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::NAPKIN_TEXTURE);
	SetShaderMaterial(SceneConstants::NAPKIN_MATERIAL);
	SetTextureUVScale(0.9f, 0.9f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 0.03f, 3.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 13.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.25f, 0.34f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::NAPKIN_TEXTURE);
	SetShaderMaterial(SceneConstants::NAPKIN_MATERIAL);
	SetTextureUVScale(0.9f, 0.9f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
}


// Handles the book object(s)
void SceneManager::RenderBooks()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.5f, 1.50f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 115.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.80f, 1.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(0.6f, 0.6f);
	SetShaderTexture(SceneConstants::BOTTOMBOOK_TEXTURE);
	SetShaderMaterial(SceneConstants::BOTTOMBOOK_MATERIAL);

	// draw the mesh with transformation values 
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.25f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 110.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 2.20f, 1.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(0.7f, 0.7f);
	SetShaderTexture(SceneConstants::MIDDLEBOOK_TEXTURE);
	SetShaderMaterial(SceneConstants::MIDDLEBOOK_MATERIAL);

	// draw the mesh with transformation values 
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 0.55f, 5.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 100.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.45f, 3.10f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture(SceneConstants::TOPBOOK_TEXTURE);
	SetShaderMaterial(SceneConstants::TOPBOOK_MATERIAL);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
}

// Handles the sandwich object
void SceneManager::RenderSandwich()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 0.2f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.5f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADBASE_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);
	SetTextureUVScale(0.6f, 0.6f);
	// draw the mesh with transformation values 
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 0.2f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.4f, 5.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADBASE_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);
	SetTextureUVScale(0.6f, 0.6f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, true);
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 0.2f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.2f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADBASE_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);
	SetTextureUVScale(0.6f, 0.6f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 0.2f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.1f, 5.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.0f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADBASE_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);
	SetTextureUVScale(0.6f, 0.6f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, true);
	/******************************************************************/

	// Cheese
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.35f, 0.1f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.35f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.5f, 1.0f, 1.0f);
	SetShaderTexture(SceneConstants::CHEESE_TEXTURE);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// Crust
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.25f, 0.1998f, 2.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.5f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADCRUST_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.24f, 0.1998f, 0.64f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.4f, 5.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADCRUST_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);;

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.25f, 0.1998f, 2.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.2f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADCRUST_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.24f, 0.1998f, 0.64f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.1f, 5.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture(SceneConstants::BREADCRUST_TEXTURE);
	SetShaderMaterial(SceneConstants::BREAD_MATERIAL);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/
}

