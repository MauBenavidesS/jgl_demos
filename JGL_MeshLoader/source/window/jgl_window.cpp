#include "pch.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "jgl_window.h"
#include "elems/input.h"

static void on_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  JGLWindow* pWindow = static_cast<JGLWindow*>(glfwGetWindowUserPointer(window));
  pWindow->on_key(key, scancode, action, mods);
}

static void on_window_size_callback(GLFWwindow* window, int width, int height)
{
  JGLWindow* pWindow = static_cast<JGLWindow*>(glfwGetWindowUserPointer(window));
  pWindow->on_resize(width, height);
}

bool JGLWindow::init(int width, int height, const std::string& title)
{ 
  mIsValid = true;

  mWidth = width;
  mHeight = height;

  /* Initialize the library */
  if (!glfwInit())
  {
    fprintf(stderr, "Error: GLFW Window couldn't be initialized\n");
    mIsValid = false;
  }

  // Create the window and store this window as window pointer
  // so that we can use it in callback functions
  mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (!mWindow)
  {
    fprintf(stderr, "Error: GLFW Window couldn't be created\n");
    mIsValid = false;
  }

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetKeyCallback(mWindow, on_key_callback);

  glfwSetWindowSizeCallback(mWindow, on_window_size_callback);
  glfwMakeContextCurrent(mWindow);
  
  glfwSwapInterval(1); // Enable vsync


  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    mIsValid = false;
  }

  glEnable(GL_DEPTH_TEST);

  mUICtx->init(mWindow);

  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 410";

  auto aspect = (float)width / (float)height;
  mShader = std::make_unique<Shader>();
  mShader->load("shaders/vs.shader", "shaders/fs.shader");

  mCamera = std::make_unique<Camera>(glm::vec3(0, 0, 3), 45.0f, aspect, 0.1f, 100.0f);

  mLight = std::make_unique<Light>();

  load_mesh();

  mShader->use();

  glm::vec4 vColor{ 0.7f, 0.1f, 0.1f, 1.0f };
  mShader->set_vec4(vColor, "color");

  return mIsValid;
}

JGLWindow::~JGLWindow()
{
  mUICtx->end();

  if (mIsValid)
  {
    glfwDestroyWindow(mWindow);
    glfwTerminate();
  }

  if (mShader)
  {
    mShader->unload();
  }
}

void JGLWindow::on_resize(int width, int height)
{
  mWidth = width;
  mHeight = height;

  mCamera->set_aspect((float)width / (float)height);
  glViewport(0, 0, width, height);
}

void JGLWindow::on_key(int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
  }
}

void JGLWindow::render()
{
  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(mWindow))
  {
    // Open GL render context / frame
    // TODO: Move to OpenGL render context
    glViewport(0, 0, mWidth, mHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Vice versa: the elements have to know their shader
    mShader->update_camera(mCamera.get());
    mShader->update_light(mLight.get());

    // TODO: render all meshes / models here
    if (mMesh)
    {
      mMesh->render();
    }

    mUICtx->render();

    handle_input();

    glfwPollEvents();
    glfwSwapBuffers(mWindow);
  }

  mShader->unload();
}

void JGLWindow::handle_input()
{
  if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
  {
    mCamera->set_distance(-0.1f);
  }

  if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
  {
    mCamera->set_distance(0.1f);
  }
 
  double x, y;
  glfwGetCursorPos(mWindow, &x, &y);

  mCamera->on_mouse_move(x, y, Input::GetPressedButton(mWindow));
  
}

bool JGLWindow::load_mesh()
{
  Assimp::Importer Importer;

  const aiScene* pScene = Importer.ReadFile(mModel.c_str(),
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType);

  if (pScene->HasMeshes())
  {
    auto* mesh = pScene->mMeshes[0];

    mMesh = std::make_unique<Mesh>();

    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
      VertexHolder vh;
      vh.mPos    = { mesh->mVertices[i].x, mesh->mVertices[i].y ,mesh->mVertices[i].z };
      vh.mNormal = { mesh->mNormals[i].x, mesh->mNormals[i].y ,mesh->mNormals[i].z };

      mMesh->add_vertex(vh);
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];

      for (size_t j = 0; j < face.mNumIndices; j++)
        mMesh->add_vertex_index(face.mIndices[j]);
    }
  }

  mMesh->init();
  return true;
}
