/*
 * Author: Emoy Kim
 * E-mail: emoy.kim_AT_gmail.com
 * 
 * This code is a free software; it can be freely used, changed and redistributed.
 * If you use any version of the code, please reference the code.
 * 
 */

#pragma once

#include <OpenCVLinker.h>
#include <OpenGLLinker.h>

using namespace std;
using namespace cv;

class ShaderGL
{
public:
	GLuint ShaderProgram;
	GLint MVPLocation, ColorLocation, TextureLocation;

	ShaderGL();

	void setShader(const GLchar* const vertex_source, const GLchar* const fragment_source);
};

class ObjectGL
{
	vector<GLfloat> DataBuffer; // 3 for vertex and 2 for texture

	void prepareTexture2DFromFile(const string& file_name) const;
	void prepareTexture(const int& n_bytes_per_vertex, const string& texture_file_name);
	void prepareVertexBuffer(const int& n_bytes_per_vertex);
	GLvoid* bufferOffset(int offset) const { return reinterpret_cast<GLvoid *>(offset); }

public:
	GLuint ObjVAO, ObjVBO;
	GLenum DrawMode;
	GLuint TextureID;
	GLsizei VerticesCount;
	glm::vec3 Colors;

	ObjectGL();

	void setObject(
		GLenum draw_mode, 
		const glm::vec3& color, 
		const vector<glm::vec3>& vertices
	);

	void setObject(
		GLenum draw_mode, 
		const glm::vec3& color, 
		const vector<glm::vec3>& vertices,
		const vector<glm::vec2>& textures, 
		const string& texture_file_name
	);
};

class VirtualFenceMakerGL
{
	struct Camera
	{
		int Width;
		int Height;
		float FocalLength;
		float PanAngle;
		float TiltAngle;
		float CameraHeight;
		glm::vec3 CameraPosition;
		glm::mat4 PanningToCamera;
		glm::mat4 TiltingToCamera;
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ToWorldCoordinate;
		
		Camera() : Width( 0 ), Height( 0 ), FocalLength( 0.0f ), PanAngle( 0.0f ), TiltAngle( 0.0f ), CameraHeight( 0.0f ),
			CameraPosition{}, PanningToCamera{}, TiltingToCamera{}, ViewMatrix{}, ProjectionMatrix{}, ToWorldCoordinate{} {}
	};

	static VirtualFenceMakerGL* Renderer;
	GLFWwindow* RenderWindow;

	Point ClickedPoint;
	bool DrawFenceOnGroundOnly;

	Mat GroundImage;
	Mat FenceMask;
	float ActualGroundWidth; 
	float ActualGroundHeight;
	float FenceHeight;
	float FenceRadius;
	Camera MainCamera;

	ShaderGL GroundShader;
	ShaderGL FenceShader;
	ObjectGL Ground;
	ObjectGL Fence;

	bool getWorldPoint(glm::vec3& fence_center, const float& height_from_ground) const;
	void updateFenceHeight(const double& mouse_wheel_y_offset);
	void updateFenceRadius(const double& mouse_wheel_y_offset);

	void captureFenceMask();
	void drawGround();
	void drawFenceAtCenter(const glm::vec3& center);
	void render();

	void setFenceObject();
	void setGroundObject();
	void setFenceShader();
	void setGroundShader();
	void registerCallbacks() const;
	void initializeOpenGL();
	void initialize();

	void printOpenGLInformation() const;
	
	void cleanup(GLFWwindow* window);
	void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
	void mouse(GLFWwindow* window, int button, int action, int mods);
	void mousewheel(GLFWwindow* window, double xoffset, double yoffset);
	void reshape(GLFWwindow* window, int width, int height);
	void error(int error, const char* description) const;
	static void cleanupWrapper(GLFWwindow* window);
	static void keyboardWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseWrapper(GLFWwindow* window, int button, int action, int mods);
	static void mousewheelWrapper(GLFWwindow* window, double xoffset, double yoffset);
	static void reshapeWrapper(GLFWwindow* window, int width, int height);
	static void errorWrapper(int error, const char* description);
	

public:
	VirtualFenceMakerGL(const VirtualFenceMakerGL&) = delete;
	VirtualFenceMakerGL(const VirtualFenceMakerGL&&) = delete;
	VirtualFenceMakerGL& operator=(const VirtualFenceMakerGL&) = delete;
	VirtualFenceMakerGL& operator=(const VirtualFenceMakerGL&&) = delete;

	VirtualFenceMakerGL(const float& actual_width, const float& actual_height);
	~VirtualFenceMakerGL();

	void setCamera(
		const int& width, 
		const int& height, 
		const float& focal_length, 
		const float& pan_angle_in_degree,
		const float& tilt_angle_in_degree,
		const float& camera_height_in_meter
	);
	void renderFence();
};