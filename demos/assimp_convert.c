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

	// Trying to figure out why I get a different number of verts and faces than it's supposed to have
	// according to the website
	// https://graphics.stanford.edu/data/3Dscanrep/
	// Size of reconstruction: 35947 vertices, 69451 triangles
	// I get                   34835 vertices, 69666 triangles
	// with the import line above. With the one below I get the "correct" numbers but
	// the program hangs in my compute_normals() function because (I assume) my
	// code assumes a perfectly closed mesh with no holes.  TODO
	//const struct aiScene* scene = aiImportFile(argv[1], 0);

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
		if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
			puts("Warning, mesh contrained non-triangles, skipping!");
			continue;
		}

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

