#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags


#include <stdio.h>





int main(int argc, char** argv)
{
	if (argc != 3) {
		printf("Usage: %s modelfile outputfile\n\nOutput file is a flat text format:\n", argv[0]);
		printf("num_vertices\n(3.200, 5.233, 1.038)\n...\nnum_triangles\n(0, 1, 2)\n ...\n\n");
		return 0;
	}

	// Start the import on the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll t
	// probably to request more postprocessing than we do in this example.
	const struct aiScene* scene = aiImportFile(argv[1], aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

	// If the import failed, report it
	if (!scene) {
		//DoTheErrorLogging(aiGetErrorString());
		printf("Error: %s\n", aiGetErrorString());
		return 0;
	}
	
	FILE* ofile = fopen(argv[2], "w");
	if (!ofile) {
		perror("Error opening output file");
		return 0;
	}
	

	struct aiNode* nd = scene->mRootNode;
	struct aiMesh* mesh;
	struct aiVector3D tmp;
	struct aiFace* face;
	unsigned int* indices;

	printf("num meshes = %d %d\n", nd->mNumMeshes, scene->mNumMeshes);
	for (int j=0; j<scene->mNumMeshes; ++j) {
		mesh = scene->mMeshes[j];

		//for now only support triangles TODO?
		if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
			continue;

		printf("mesh #%d with %u verts %u faces\n", j, mesh->mNumVertices, mesh->mNumFaces);
		//getchar();
		//getchar();

		fprintf(ofile, "%u\n", mesh->mNumVertices);
		for (int i=0; i<mesh->mNumVertices; ++i) {
			tmp = mesh->mVertices[i];
			fprintf(ofile, "(%f, %f, %f)\n", tmp.x, tmp.y, tmp.z);
		}

		fprintf(ofile, "%u\n", mesh->mNumFaces);
		for (int i=0; i<mesh->mNumFaces; ++i) {
			face = &mesh->mFaces[i];
			indices = face->mIndices;

			fprintf(ofile, "(%u, %u, %u)\n", indices[0], indices[1], indices[2]);

		}
		
	}

	// We're done. Release all resources associated with this import
	aiReleaseImport(scene);

	fclose(ofile);

	return 0;

}

