#include "VirtualFenceMakerGL.h"

//------------------------------------------------------------------
//
// Shader Class
//
//------------------------------------------------------------------

ShaderGL::ShaderGL() : ShaderProgram( 0 ), MVPLocation( 0 ), ColorLocation( 0 ), TextureLocation( 0 )
{
}

void ShaderGL::setShader(const GLchar* const vertex_source, const GLchar* const fragment_source)
{
   const GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
   const GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );

   glShaderSource( vertex_shader, 1, &vertex_source, nullptr );
   glShaderSource( fragment_shader, 1, &fragment_source, nullptr );
   glCompileShader( vertex_shader );
   glCompileShader( fragment_shader );

   ShaderProgram = glCreateProgram();
   glAttachShader( ShaderProgram, vertex_shader );
   glAttachShader( ShaderProgram, fragment_shader );
   glLinkProgram( ShaderProgram );

   MVPLocation = glGetUniformLocation( ShaderProgram, "ModelViewProjectionMatrix" );
   ColorLocation = glGetUniformLocation( ShaderProgram, "PrimitiveColor" );
   TextureLocation = glGetUniformLocation( ShaderProgram, "BaseTexture" );

   glDeleteShader( vertex_shader );
   glDeleteShader( fragment_shader );
}


//------------------------------------------------------------------
//
// Object Class
//
//------------------------------------------------------------------

ObjectGL::ObjectGL() : ObjVAO( 0 ), ObjVBO( 0 ), DrawMode( 0 ), TextureID( 0 ), VerticesCount( 0 ), Colors{}
{
}

void ObjectGL::prepareVertexBuffer(int n_bytes_per_vertex)
{
   glGenBuffers( 1, &ObjVBO );
   glBindBuffer( GL_ARRAY_BUFFER, ObjVBO );
   glBufferData( GL_ARRAY_BUFFER, sizeof(GLfloat) * DataBuffer.size(), DataBuffer.data(), GL_STATIC_DRAW );
   glBindBuffer( GL_ARRAY_BUFFER, 0 );

   glGenVertexArrays( 1, &ObjVAO );
   glBindVertexArray( ObjVAO );
   glBindBuffer( GL_ARRAY_BUFFER, ObjVBO );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, bufferOffset( 0 ) );
   glEnableVertexAttribArray( 0 );
}

void ObjectGL::prepareTexture2DFromFile(const std::string& file_name) const
{
   const FREE_IMAGE_FORMAT format = FreeImage_GetFileType( file_name.c_str(), 0 );
   FIBITMAP* texture = FreeImage_Load( format, file_name.c_str() );
   const uint n_bits_per_pixel = FreeImage_GetBPP( texture );
   
   FIBITMAP* texture_32bit;
   if (n_bits_per_pixel == 32) {
      texture_32bit = texture;
   }
   else {
      texture_32bit = FreeImage_ConvertTo32Bits( texture );
   }

   const uint width = FreeImage_GetWidth( texture_32bit );
   const uint height = FreeImage_GetHeight( texture_32bit );
   GLvoid* data = FreeImage_GetBits( texture_32bit );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data );
   
   FreeImage_Unload( texture_32bit );
   if (n_bits_per_pixel != 32) {
      FreeImage_Unload( texture );
   }
}

void ObjectGL::prepareTexture(int n_bytes_per_vertex, const std::string& texture_file_name)
{
   glGenTextures( 1, &TextureID );
   glActiveTexture( GL_TEXTURE0 + TextureID );
   glBindTexture( GL_TEXTURE_2D, TextureID );
   
   prepareTexture2DFromFile( texture_file_name );
   
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

   glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, bufferOffset( 3 * sizeof(GLfloat) ) );
   glEnableVertexAttribArray( 2 );
}

void ObjectGL::setObject(GLenum draw_mode, const glm::vec3& color, const std::vector<glm::vec3>& vertices)
{
   DrawMode = draw_mode;
   Colors = { color.r, color.g, color.b };
   for (auto const& vertex : vertices) {
      DataBuffer.push_back( vertex.x );
      DataBuffer.push_back( vertex.y );
      DataBuffer.push_back( vertex.z );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 3 * sizeof(GLfloat);
   prepareVertexBuffer( n_bytes_per_vertex );
}

void ObjectGL::setObject(
   GLenum draw_mode, 
   const glm::vec3& color, 
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec2>& textures, 
   const std::string& texture_file_name
)
{
   DrawMode = draw_mode;
   Colors = { color.r, color.g, color.b };
   for (uint i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 5 * sizeof(GLfloat);
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareTexture( n_bytes_per_vertex, texture_file_name );
}


//------------------------------------------------------------------
//
// Renderer Class
//
//------------------------------------------------------------------

VirtualFenceMakerGL::VirtualFenceMakerGL(float actual_width, float actual_height) :
   RenderWindow( nullptr ), ClickedPoint( -1, -1 ), DrawFenceOnGroundOnly( false ), FenceMask( nullptr ),
   ActualGroundWidth( actual_width ), ActualGroundHeight( actual_height ), FenceHeight( 20.0f ), FenceRadius( 20.0f )
{
   Renderer = this;

   setCamera( 1280, 720, 800.0f, 0.0f, 20.0f, 70.0f );

   initialize();
   printOpenGLInformation();
}

VirtualFenceMakerGL::~VirtualFenceMakerGL()
{
   delete [] FenceMask;

   glfwTerminate();
}

void VirtualFenceMakerGL::printOpenGLInformation()
{
   std::cout << "****************************************************************\n";
   std::cout << " - GLFW version supported: " << glfwGetVersionString() << "\n";
   std::cout << " - OpenGL renderer: " << glGetString( GL_RENDERER ) << "\n";
   std::cout << " - OpenGL version supported: " << glGetString( GL_VERSION ) << "\n";
   std::cout << " - OpenGL shader version supported: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";
   std::cout << "****************************************************************" << "\n\n";
}

void VirtualFenceMakerGL::initializeOpenGL()
{
   if (!glfwInit()) {
      exit( EXIT_FAILURE );
   }
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
   glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   RenderWindow = glfwCreateWindow( MainCamera.Width, MainCamera.Height, "Main Camera", nullptr, nullptr );
   if (!RenderWindow) {
      glfwTerminate();
      exit( EXIT_FAILURE );
   }
   glfwMakeContextCurrent( RenderWindow );

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
   }

   glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
}

void VirtualFenceMakerGL::setCamera(
   int width, 
   int height, 
   float focal_length, 
   float pan_angle_in_degree,
   float tilt_angle_in_degree,
   float camera_height_in_meter
)
{
   MainCamera.Width = width;
   MainCamera.Height = height;
   MainCamera.FocalLength = focal_length;
   MainCamera.PanAngle = glm::radians( pan_angle_in_degree );
   MainCamera.TiltAngle = glm::radians( tilt_angle_in_degree );

   MainCamera.CameraHeight = camera_height_in_meter;
   MainCamera.CameraPosition = glm::vec3(0.0f);

   MainCamera.PanningToCamera = glm::rotate( glm::mat4(1.0f), MainCamera.PanAngle, glm::vec3(0.0f, -1.0f, 0.0f) );
   MainCamera.TiltingToCamera = glm::rotate( glm::mat4(1.0f), MainCamera.TiltAngle, glm::vec3(1.0f, 0.0f, 0.0f) );
   
   MainCamera.ToWorldCoordinate = 
      translate( glm::mat4(1.0f), MainCamera.CameraPosition ) *
      inverse( MainCamera.PanningToCamera ) * inverse( MainCamera.TiltingToCamera );

   const glm::vec4 viewing_point = MainCamera.ToWorldCoordinate * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
   const glm::vec4 up_vector = MainCamera.ToWorldCoordinate * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
   MainCamera.ViewMatrix = lookAt( MainCamera.CameraPosition, glm::vec3(viewing_point), glm::vec3(up_vector) );

   const auto fovy = 2.0f * atan( static_cast<float>(height) / (2.0f * focal_length) );
   const auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
   MainCamera.ProjectionMatrix = glm::perspective( fovy, aspect_ratio, 1.0f, 10000.0f );

   const int size = width * height * sizeof( uint8_t );
   if (FenceMask == nullptr) FenceMask = (uint8_t*)malloc( size );
}

void VirtualFenceMakerGL::cleanup(GLFWwindow* window)
{
   glDeleteProgram( GroundShader.ShaderProgram );
   glDeleteProgram( FenceShader.ShaderProgram );

   glDeleteVertexArrays( 1, &Ground.ObjVAO );
   glDeleteVertexArrays( 1, &Fence.ObjVAO );
   
   glDeleteBuffers( 1, &Ground.ObjVBO );
   glDeleteBuffers( 1, &Fence.ObjVBO );

   glfwSetWindowShouldClose( window, GLFW_TRUE );
}

void VirtualFenceMakerGL::cleanupWrapper(GLFWwindow* window)
{
   Renderer->cleanup( window );
}

void VirtualFenceMakerGL::captureFenceMask() const
{
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_BACK );
   glReadPixels( 0, 0, MainCamera.Width, MainCamera.Height, GL_RED, GL_UNSIGNED_BYTE, FenceMask );

   const int size = MainCamera.Width * MainCamera.Height;
   for (int i = 0; i < size; ++i) {
      if (FenceMask[i] == 255) FenceMask[i] = 0;
      else if (FenceMask[i] != 0) FenceMask[i] = 255;
   }

   FIBITMAP* fence_image = FreeImage_ConvertFromRawBits(
      FenceMask,
      MainCamera.Width,
      MainCamera.Height,
      MainCamera.Width,
      8,
      0, 
      0, 
      0,
      false
   );
   FreeImage_Save( FIF_PNG, fence_image, std::string(std::string(CMAKE_SOURCE_DIR) + "/fence_mask.png").c_str() );
   FreeImage_Unload( fence_image );
   std::cout << "Fence Mask Saved!\n";
}

void VirtualFenceMakerGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_C:
         DrawFenceOnGroundOnly = true;
         render();
         captureFenceMask();
         DrawFenceOnGroundOnly = false;
         break;
      case GLFW_KEY_R:
         DrawFenceOnGroundOnly = !DrawFenceOnGroundOnly;
         break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanupWrapper( RenderWindow );
         break;
      default:
         return;
   }
}

void VirtualFenceMakerGL::keyboardWrapper(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   Renderer->keyboard( window, key, scancode, action, mods );
}

void VirtualFenceMakerGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      double x, y;
      glfwGetCursorPos( RenderWindow, &x, &y );
      ClickedPoint.x = static_cast<int>(round( x ));
      ClickedPoint.y = static_cast<int>(round( y ));
   }
}

void VirtualFenceMakerGL::mouseWrapper(GLFWwindow* window, int button, int action, int mods)
{
   Renderer->mouse( window, button, action, mods );
}

void VirtualFenceMakerGL::updateFenceHeight(double mouse_wheel_y_offset)
{
   if (mouse_wheel_y_offset >= 0.0) {
      FenceHeight += 5.0f;
      if (FenceHeight >= 70.0f) FenceHeight -= 5.0f; 
   }
   else {
      FenceHeight -= 5.0f;
      if (FenceHeight < 0.0f) FenceHeight = 0.0f;
   }
}

void VirtualFenceMakerGL::updateFenceRadius(double mouse_wheel_y_offset)
{
   if (mouse_wheel_y_offset >= 0.0) {
      FenceRadius += 5.0f;
      if (FenceRadius >= ActualGroundHeight * 0.5f) FenceRadius -= 5.0f; 
   }
   else {
      FenceRadius -= 5.0f;
      if (FenceRadius < 5.0f) FenceRadius = 5.0f;
   }
}

void VirtualFenceMakerGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset)
{
   if (ClickedPoint.x >= 0) {
      const int state = glfwGetKey( RenderWindow, GLFW_KEY_LEFT_CONTROL );
      if (state == GLFW_PRESS) updateFenceHeight( yoffset );
      else updateFenceRadius( yoffset );
   }
}

void VirtualFenceMakerGL::mousewheelWrapper(GLFWwindow* window, double xoffset, double yoffset)
{
   Renderer->mousewheel( window, xoffset, yoffset );
}

void VirtualFenceMakerGL::reshape(GLFWwindow* window, int width, int height)
{
   MainCamera.Width = width;
   MainCamera.Height = height;
   glViewport( 0, 0, width, height );
}

void VirtualFenceMakerGL::reshapeWrapper(GLFWwindow* window, int width, int height)
{
   Renderer->reshape( window, width, height );
}

void VirtualFenceMakerGL::error(int error, const char* description) const
{
   puts( description );
}

void VirtualFenceMakerGL::errorWrapper(int error, const char* description)
{
   Renderer->error( error, description );
}

void VirtualFenceMakerGL::registerCallbacks() const
{
   glfwSetErrorCallback( errorWrapper );
   glfwSetWindowCloseCallback( RenderWindow, cleanupWrapper );
   glfwSetKeyCallback( RenderWindow, keyboardWrapper );
   glfwSetMouseButtonCallback( RenderWindow, mouseWrapper );
   glfwSetScrollCallback( RenderWindow, mousewheelWrapper );
   glfwSetFramebufferSizeCallback( RenderWindow, reshapeWrapper );
}

void VirtualFenceMakerGL::setGroundShader()
{
   const GLchar* const vertex_source = {
      "#version 330                                             \n"
      "uniform mat4 ModelViewProjectionMatrix;                  \n"
      "uniform vec3 PrimitiveColor;                             \n"
      "layout (location = 0) in vec4 v_position;                \n"
      "layout (location = 2) in vec2 v_tex_coord;               \n"
      "out vec4 color;                                          \n"
      "out vec2 tex_coord;                                      \n"
      "void main(void) {                                        \n"
      "	color = vec4( PrimitiveColor, 1.0f );                  \n"
      "	tex_coord = v_tex_coord;                               \n"
      "	gl_Position =  ModelViewProjectionMatrix * v_position; \n"
      "}                                                        \n"
   };
   const GLchar* const fragment_source = {
      "#version 330                                       \n"
      "uniform sampler2D BaseTexture;                     \n"
      "in vec4 color;                                     \n"
      "in vec2 tex_coord;                                 \n"
      "layout (location = 0) out vec4 final_color;        \n"
      "void main(void) {                                  \n"
      "	final_color = texture( BaseTexture, tex_coord ); \n"
      "}                                                  \n"
   };

   GroundShader.setShader( vertex_source, fragment_source );
}

void VirtualFenceMakerGL::setFenceShader()
{
   const GLchar* const vertex_source = {
      "#version 330                                             \n"
      "uniform mat4 ModelViewProjectionMatrix;                  \n"
      "uniform vec3 PrimitiveColor;                             \n"
      "layout (location = 0) in vec4 v_position;                \n"
      "out vec4 color;                                          \n"
      "void main(void) {                                        \n"
      "	color = vec4( PrimitiveColor, 1.0f );                  \n"
      "	gl_Position =  ModelViewProjectionMatrix * v_position; \n"
      "}                                                        \n"
   };
   const GLchar* const fragment_source = {
      "#version 330                                \n"
      "in vec4 color;                              \n"
      "layout (location = 0) out vec4 final_color; \n"
      "void main(void) {                           \n"
      "	final_color = color;                      \n"
      "}                                           \n"
   };

   FenceShader.setShader( vertex_source, fragment_source );
}

void VirtualFenceMakerGL::setFenceObject()
{
   const glm::vec3 fence_color = { 0.5f, 0.125f, 0.9f };
   std::vector<glm::vec3> fence_vertices;
   fence_vertices.emplace_back( 0.0f, 0.0f, 0.0f );
   for (int theta = 0; theta <= 360; theta += 5) {
      const auto rad = glm::radians( static_cast<float>(theta) );
      fence_vertices.emplace_back( cosf( rad ), 0.0f, sinf( rad ) );
   }
   Fence.setObject( GL_TRIANGLE_FAN, fence_color, fence_vertices );
}

void VirtualFenceMakerGL::setGroundObject()
{
   const glm::vec3 ground_color = { 0.0f, 1.0f, 0.0f };
   const std::vector<glm::vec3> ground_vertices = {
      glm::vec3(0.0f, MainCamera.CameraHeight, 0.0f),
      glm::vec3(0.0f, MainCamera.CameraHeight, ActualGroundWidth),
      glm::vec3(ActualGroundHeight, MainCamera.CameraHeight, 0.0f),
      glm::vec3(ActualGroundHeight, MainCamera.CameraHeight, 0.0f),
      glm::vec3(0.0f, MainCamera.CameraHeight, ActualGroundWidth),
      glm::vec3(ActualGroundHeight, MainCamera.CameraHeight, ActualGroundWidth)
   };
   const std::vector<glm::vec2> ground_textures = {
      glm::vec2(0.0f, 1.0f),
      glm::vec2(1.0f, 1.0f),
      glm::vec2(0.0f, 0.0f),
      glm::vec2(0.0f, 0.0f),
      glm::vec2(1.0f, 1.0f),
      glm::vec2(1.0f, 0.0f)
   };
   Ground.setObject( 
      GL_TRIANGLES, 
      ground_color, 
      ground_vertices, 
      ground_textures, 
      std::string(CMAKE_SOURCE_DIR) + "/ground.jpg" 
   );
}

void VirtualFenceMakerGL::initialize()
{
   initializeOpenGL();
   registerCallbacks();

   setGroundShader();
   setFenceShader();
   setGroundObject();
   setFenceObject();
}

bool VirtualFenceMakerGL::getWorldPoint(glm::vec3& fence_center, float height_from_ground) const
{
   const auto x = static_cast<float>(ClickedPoint.x);
   const auto y = static_cast<float>(ClickedPoint.y);
   const auto half_width = static_cast<float>(MainCamera.Width) * 0.5f;
   const auto half_height = static_cast<float>(MainCamera.Height) * 0.5f;
   const float sin_tilt = sinf( MainCamera.TiltAngle );
   const float cos_tilt = cosf( MainCamera.TiltAngle );
   const float f_mul_sin_tilt = MainCamera.FocalLength * sin_tilt;

   glm::vec3 ground_point;
   ground_point.z = f_mul_sin_tilt + (y - half_height) * cos_tilt;

   if (ground_point.z <= 0.0 || MainCamera.CameraHeight < height_from_ground) return false;

   ground_point.z = (MainCamera.CameraHeight - height_from_ground) / ground_point.z;
   ground_point.x = (x - half_width) * ground_point.z;
   ground_point.y = (y - half_height) * ground_point.z;
   ground_point.z = MainCamera.FocalLength * ground_point.z;

   const glm::vec4 world_point = MainCamera.ToWorldCoordinate * glm::vec4(ground_point, 1.0f);
   fence_center = glm::vec3(world_point);
   return true;
}

void VirtualFenceMakerGL::drawGround()
{
   const glm::mat4 model_view_projection = MainCamera.ProjectionMatrix * MainCamera.ViewMatrix;

   glUseProgram( GroundShader.ShaderProgram );
   glUniformMatrix4fv( GroundShader.MVPLocation, 1, GL_FALSE, &model_view_projection[0][0] );
   glUniform1i( GroundShader.TextureLocation, Ground.TextureID );

   glBindVertexArray( Ground.ObjVAO );
   glUniform3fv( GroundShader.ColorLocation, 1, value_ptr( Ground.Colors ) );
   glDrawArrays( Ground.DrawMode, 0, Ground.VerticesCount );
   glBindVertexArray( 0 );
}

void VirtualFenceMakerGL::drawFenceAtCenter(const glm::vec3& center)
{
   glm::mat4 model_view_projection = MainCamera.ProjectionMatrix * MainCamera.ViewMatrix;

   glUseProgram( FenceShader.ShaderProgram );
   const glm::mat4 to_center = scale( translate( glm::mat4(1.0f), center ), glm::vec3(FenceRadius) );
   model_view_projection = model_view_projection * to_center;
   glUniformMatrix4fv( FenceShader.MVPLocation, 1, GL_FALSE, &model_view_projection[0][0] );
   glUniform1i( FenceShader.TextureLocation, Ground.TextureID );

   glBindVertexArray( Fence.ObjVAO );
   glUniform3fv( FenceShader.ColorLocation, 1, value_ptr( Fence.Colors ) );
   glDrawArrays( Fence.DrawMode, 0, Fence.VerticesCount );
   glBindVertexArray( 0 );
}

void VirtualFenceMakerGL::render()
{
   glClear( OPENGL_COLOR_BUFFER_BIT );

   if (!DrawFenceOnGroundOnly) drawGround();

   glm::vec3 fence_center;
   if (ClickedPoint.x >= 0 && getWorldPoint( fence_center, FenceHeight )) {
      if (!DrawFenceOnGroundOnly) drawFenceAtCenter( fence_center );
      
      fence_center.y = MainCamera.CameraHeight;
      drawFenceAtCenter( fence_center );
   }

   glUseProgram( 0 );
}

void VirtualFenceMakerGL::renderFence()
{
   if (glfwWindowShouldClose( RenderWindow )) initialize();
   
   while (!glfwWindowShouldClose( RenderWindow )) {
      render();
      
      glfwPollEvents();
      glfwSwapBuffers( RenderWindow );
   }
   glfwDestroyWindow( RenderWindow );
}