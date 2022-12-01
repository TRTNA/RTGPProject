#include <utils/model.h>
#include <iostream>
using std::cout;
using std::endl;
using std::string;
using std::vector;

Model::Model(const string &path)
{
    this->loadModel(path);
}

void Model::Draw()
{
    for (GLuint i = 0; i < this->meshes.size(); i++)
        this->meshes[i]->Draw();
}

void Model::loadModel(const string &path)
{

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

    // check for errors (see comment above)
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }

    this->processNode(scene->mRootNode, scene);

    // MATERIALS
    if (!scene->HasMaterials())
    {
        cout << "Model has not any material" << endl;
        return;
    }
}

// Recursive processing of nodes of Assimp data structure
void Model::processNode(aiNode *node, const aiScene *scene)
{
    // we process each mesh inside the current node
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {

        // the "node" object contains only the indices to objects in the scene
        // "Scene" contains all the data. Class node is used only to point to one or more mesh inside the scene and to maintain informations on relations between nodes
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // we start processing of the Assimp mesh using processMesh method.
        // the result (an istance of the Mesh class) is added to the vector
        // we use emplace_back instead as push_back, so to have the instance created directly in the
        // vector memory, without the creation of a temp copy.
        // https://en.cppreference.com/w/cpp/container/vector/emplace_back
        Mesh *m = processMesh(scene, mesh);
        this->meshes.emplace_back(m);
    }
    // we then recursively process each of the children nodes
    for (GLuint i = 0; i < node->mNumChildren; i++)
    {
        this->processNode(node->mChildren[i], scene);
    }
}

//////////////////////////////////////////

// Processing of the Assimp mesh in order to obtain an "OpenGL mesh"
// = we create and allocate the buffers used to send mesh data to the GPU
Mesh *Model::processMesh(const aiScene *scene, aiMesh *mesh)
{
    // data structures for vertices and indices of vertices (for faces)
    vector<Vertex> vertices;
    vector<GLuint> indices;

    for (GLuint i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // the vector data type used by Assimp is different than the GLM vector needed to allocate the OpenGL buffers
        // I need to convert the data structures (from Assimp to GLM, which are fully compatible to the OpenGL)
        glm::vec3 vector;
        // vertices coordinates
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // Normals
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
        // Texture Coordinates
        // if the model has texture coordinates, than we assign them to a GLM data structure, otherwise we set them at 0
        // if texture coordinates are present, than Assimp can calculate tangents and bitangents, otherwise we set them at 0 too

        if (mesh->HasTextureCoords(0))
        {
            glm::vec2 vec;
            // in this example we assume the model has only one set of texture coordinates. Actually, a vertex can have up to 8 different texture coordinates. For other models and formats, this code needs to be adapted and modified.
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            cout << "WARNING::ASSIMP:: MODEL WITHOUT UV COORDINATES -> TANGENT AND BITANGENT ARE = 0" << endl;
        }
        if (mesh->HasTangentsAndBitangents())
        {
            // Tangents
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // Bitangents
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;

            // we add the vertex to the list
            vertices.emplace_back(vertex);
        }
    }

    // for each face of the mesh, we retrieve the indices of its vertices , and we store them in a vector data structure
    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (GLuint j = 0; j < face.mNumIndices; j++)
            indices.emplace_back(face.mIndices[j]);
    }


    Mesh *m = new Mesh(vertices, indices);
    return m;
}
