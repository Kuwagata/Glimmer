
/******************************************

        Breadth First Search
        Computes single-source distances for
        unweighted graphs

		NOTE:	Taken directly from Yehuda 
				Koren's "EmbedderDefs.[cpp|h]"
				origin: http://www.research.att.com/~yehuda/programs/embedder.zip

******************************************/

#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <time.h>
#include "graph.h"
#include <map>

extern float g_f_norm_dist;
clock_t g_preproc_time;

/*
	Add an edge from b to a's vertex data
*/
void add_single_edge( vtx_data *graph, const int a, const int b ) {

	int i = 0;
	for( ; i < graph[a].nedges && graph[a].edges[i]!=b ; i++ );	// perform a linear search for the node

	if( i == graph[a].nedges ) {										// if we haven't found it, add it
		graph[a].nedges++;
		graph[a].ewgts = (float *)	realloc( graph[a].ewgts, sizeof( float ) * graph[a].nedges);
		graph[a].edges = (int *)	realloc( graph[a].edges, sizeof( int ) * graph[a].nedges);
		graph[a].edges[graph[a].nedges-1]=b;
		graph[a].ewgts[graph[a].nedges-1]=1.f/g_f_norm_dist;
	}

}

/*
	add an edge to the graph between nodes number a and b
*/
void add_edge( vtx_data *graph, const int a, const int b ) {
	add_single_edge( graph, a, b );
	add_single_edge( graph, b, a );
}

/*
	generates 32-bit pseudorandom number
*/
int myrand32( ) {

	unsigned int n = (unsigned int)rand();
	unsigned int m = (unsigned int)rand();

	return ((int)((n << 16) + m));

}

void swap_helper( vtx_data *graph, int a, int b ) {
	vtx_data temp = graph[a];
	graph[a]=graph[b];
	graph[b]=temp;
}

/*
	add the edges from vertex in graph to matched vertex in cgraph
*/
void add_matched_edges( int vertex, vtx_data *cgraph, vtx_data *graph ) {
	int i;
	for( i = 0; i < graph[vertex].nedges; i++ ) {
		if( graph[vertex].match != graph[graph[vertex].edges[i]].match ) {
			add_edge( cgraph, graph[vertex].match-1, graph[graph[vertex].edges[i]].match-1 );
		}
	}
}

/*
	coarsen the graph using Henderickson and Leland's algorithm from 
	their paper:

	"A Multilevel Algorithm for Partitioning Graphs. In S. Karin, editor, 
	Proc. Supercomputing ’95, San Diego. ACM Press, New York, 1995."

	it's not optimal, but it's linear and a pretty good approximation
*/
vtx_data *coarsen_graph( vtx_data *graph, int n ) {
	int i,j;
	int new_n = 0;
	vtx_data *newgraph = NULL;

	// match each node with an unmatched node (or self if no options exist)

	for( i = 0; i < n; i++ ) {
		if( !graph[i].match ) {
			new_n++;
			for( j = 0; j < graph[i].nedges; j++ ) {
				if( !graph[(graph[i].edges[j])].match ) {
					graph[i].match = new_n;
					graph[(graph[i].edges[j])].match = new_n;
				}
				break;
			}
			if( !graph[i].match ) graph[i].match = new_n;
		}
	}

	// build new graph from old graph

	newgraph = (vtx_data *)calloc( new_n, sizeof( vtx_data ) );
	for( i = 0; i < n; i++ ) {
		add_matched_edges( i, newgraph, graph );	
	}

	return newgraph;
}


/*
	load vertex data from edge-edge file which is zero indexed
*/
vtx_data *load_vtx_data( const char *filename, int *n ) {
	
	char line[1028];
	char item[1028];
	int i,j;
	int start,end;
	int numnodes = 0;

	// first find the number of nodes ( find the largest int )

	FILE *fp = NULL;
	fp = fopen( filename, "r" );
	while( fgets( line, 1027, fp ) != NULL ) {
		j=i=0;		
		while(line[i]!=' '&&line[i]!='\t')
			item[j++]=line[i++];
		item[j]='\0';
		numnodes=atoi(item)>numnodes?atoi(item):numnodes;

		i++;
		j = 0;
		while(line[i]!='\0')
			item[j++]=line[i++];
		item[j]='\0';
		numnodes=atoi(item)>numnodes?atoi(item):numnodes;
	}	
	fclose(fp);

	numnodes++;
	*n = numnodes;
	vtx_data *graph = (vtx_data *)calloc(numnodes,sizeof(vtx_data));

	// actually read in edges
	fp = fopen( filename, "r" );

	while( fgets( line, 1027, fp ) != NULL ) {

		j=i=0;		
		while(line[i]!=' '&&line[i]!='\t')
			item[j++]=line[i++];
		item[j]='\0';

		start=atoi(item);

		i++;
		j = 0;
		while(line[i]!='\0')
			item[j++]=line[i++];
		item[j]='\0';
		//end=atoi(item)-1;
		end=atoi(item);

		if( start != end ) { // don't allow self edges
			add_edge( graph, start, end );
		}
	}	

	fclose(fp);

	////
	// permute the input nodes and their edge references
	////

	// create random permutation of graph	

	int *indices = (int *)malloc( sizeof(int) * numnodes );
	int *positions = (int *)malloc( sizeof(int) * numnodes );
	for( i = 0; i < numnodes; i++ ) {
		indices[i]=i;
	}
	for( i = 0; i < numnodes-1; i++ ) {        
		j = i+(myrand32() % (numnodes-i));
		int temp = indices[i];
		indices[ i ] = indices[ j ];
		indices[ j ] = temp;
		swap_helper( graph, i, j );
	}

	// relabel edges

	for( i = 0; i < numnodes; i++ ) {
		positions[indices[i]]=i;
	}
	for( i = 0; i < numnodes; i++ ) {
		for( j = 0; j < graph[ i ].nedges; j++ ) {
			graph[ i ].edges[ j ] = positions[ graph[ i ].edges[ j ] ];
		}
	}
	free(indices);
	free(positions);

	return graph;
}

void bfs_old(int vertex, vtx_data * localGraph, int n, DistType * dist, Queue & Q)  {
  int i;

  // initial distances with edge weights:
  for (i=0; i<n; i++) 
    dist[i]=-1;
  dist[vertex]=0;
                
  Q.initQueue(vertex);
        
  int closestVertex, neighbor;
  DistType closestDist;
  while (Q.dequeue(closestVertex)) {
    closestDist=dist[closestVertex];
    for (i=1; i<localGraph[closestVertex].nedges; i++) {
      neighbor=localGraph[closestVertex].edges[i];
      if (dist[neighbor]<-0.5) {  // first time to reach neighbor
        dist[neighbor]=closestDist+(DistType)localGraph[closestVertex].ewgts[i];
        Q.enqueue(neighbor);
      }
    }
  }
  
  // For dealing with disconnected graphs:
  for (i=0; i<n; i++)
    if (dist[i]<-0.5) // 'i' is not connected to 'vertex'
      dist[i]=closestDist+10;
}

/*
	Perform Breadth First search between a single node and all other nodes
	recording the distance in the texture array.
*/

void bfs(int vertex,			// source vertex
		 vtx_data * localGraph,	// vertex data array
		 int n,					// number of vertices
		 float * dist,			// array containing interpoint distances
		 int width,				// width of a single distance element in the texture data
		 int offset,			// offset into distance element
		 int m_offset,			// offset between index data and distance data in the texture
		 Queue & Q)  {			// queue data structure for BFS
  int i;

  // initial distances with edge weights:
  for (i=0; i<n; i++) 
    dist[i*width+offset+m_offset]=-1;
  dist[width*vertex+offset+m_offset]=0.0f;
  dist[width*vertex+offset]=(float)vertex;
                
  Q.initQueue(vertex);
        
  int closestVertex, neighbor;
  float closestDist;
  while (Q.dequeue(closestVertex)) {
    closestDist=dist[closestVertex*width+offset+m_offset];
    for (i=0; i<localGraph[closestVertex].nedges; i++) {
      neighbor=localGraph[closestVertex].edges[i];
      if (dist[neighbor*width+offset+m_offset]<-0.5) {  // first time to reach neighbor
		dist[neighbor*width+offset] = (float)vertex;
        dist[neighbor*width+offset+m_offset]=(float)(closestDist+localGraph[closestVertex].ewgts[i]);
        Q.enqueue(neighbor);
      }
    }
  }  

  // For dealing with disconnected graphs:
  for (i=0; i<n; i++)
	  if (dist[i*width+offset+m_offset]<-0.5) { // 'i' is not connected to 'vertex' 
		dist[i*width+offset] = (float)vertex;
		//dist[i*width+offset+m_offset]=(float)(closestDist)+10.f/g_f_norm_dist; 
		dist[i*width+offset+m_offset]=1000.0f; 
	  }
	
  dist[width*vertex+offset+m_offset]=1000.0f;	// ensure the zero length is ignored in force calcs
}

/**

	extract the closest local_count neighbors to a node

**/
void nneigbs(vtx_data * localGraph,		// vertex topology data
			 int n,						// number of nodes
			 float * dist,				// index and distance texture data
			 int local_count,			// number of local neighbors to extract distances from
			 int width,					// width of a distance element
			 int offset,				// offset between index and distance data in the texture
			 int *pvtmarker,			// marker that says point has been used before as a pivot
			 Queue & Q)  {				// queue data structure for depth-limited bfs

  Queue Q_local(local_count);
  int i,vertex;

  std::map<int,int> *markmap = NULL;

  // for every point, extract the closest local_count points
  for( vertex = 0; vertex < n; vertex++ ) {

	int closestVertex, neighbor;
	float closestDist;
	int closestCard;
    int numqueued = -1;

	if( markmap != NULL)
		delete markmap;	
	markmap = new std::map<int,int>();
	(*markmap)[vertex]=1;
	Q.initQueue(vertex);
	Q_local.initQueue( numqueued );        

	for( i = 0; i < local_count; i++ ) {	// initialize to ignore
		dist[vertex*width+i] = (float)((vertex+1)%n);
		dist[vertex*width+i+offset] = 1000.0f;
	}
	while (Q.dequeue(closestVertex) && Q_local.dequeue(closestCard) ) {
		closestDist = ( closestCard < 0 )?0.f:dist[vertex*width+closestCard+offset];
		for (i=0; i<localGraph[closestVertex].nedges; i++) {
			neighbor=localGraph[closestVertex].edges[i];
			std::map<int,int>::iterator iter = markmap->find(neighbor);
			if( iter == markmap->end( ) ) {  // first time to reach neighbor
				(*markmap)[neighbor]=1;
				numqueued++;
				dist[vertex*width+numqueued] = (float)neighbor;
				dist[vertex*width+numqueued+offset]=(float)closestDist+localGraph[closestVertex].ewgts[i];
				Q.enqueue(neighbor);
				Q_local.enqueue(numqueued);
				if( numqueued == local_count-1 ) {
					break;
				}
			}
	    }
		if( numqueued == local_count-1 ) {
			break;
		}
	}
	for( i = 0; i < local_count; i++ ) {	// mark pivot duplicates
		if(pvtmarker[(int)dist[vertex*width+i]])
			dist[vertex*width+i+offset] = 1000.0f;
	}
  }

}

/**
	extract_distances -		compute the set of pivots, the complete distances to those pivots and the
							nearest neighbors of each point

	graph		- the graph of the input data
	n			- number of nodes in the graph
	local_count	- number of local nodes to find
	pivot_count - number of pivots to exhaust
	tex_data	- big ol' float buffer to hold the indices and distances
	tex_offset	- offset that separates indices and distances

**/
void extract_distances( vtx_data * graph, int n, int local_count, int pivot_count, float *tex_data, int tex_offset ) {

	int i,j;
	
	int * pvtmarker = new int[n];
	memset( pvtmarker, 0, sizeof(int)*n );

	float * dist = new float[n]; // this vector stores  the distances of each nodes to the selected "pivots"

	// select the first pivot
	int node = myrand32()%n;
	pvtmarker[node]=1;

	clock_t clock1 = clock( );

	Queue Q(n);
	bfs(node, graph, n, tex_data, pivot_count+local_count, local_count, tex_offset, Q);

	//DistType * dist_old = new DistType[n];
	//for(i = 0; i < 50; i++ ) {
	//	bfs(node, graph, n, tex_data, pivot_count+local_count, local_count, tex_offset, Q);
	//	//bfs_old(node, graph, n, dist_old, Q);
	//}
	//printf("bfs time = %d\n", clock()-clock1 );
	//delete dist_old;
	//exit( 0 );

	float max_dist=0;
	for (i=0; i<n; i++) {
		dist[i]=tex_data[(pivot_count+local_count)*i+local_count+tex_offset];
		if( dist[i] == 1000.0f ) dist[i] = 0.0f;
		if (dist[i]>max_dist) {
			node=i;
			max_dist=dist[i];
		}
	}
	
	// select other pivot_count-1 nodes as pivots

	for (i=1; i<pivot_count; i++) {
		pvtmarker[node]=1;
		bfs(node, graph, n, tex_data, pivot_count+local_count, local_count+i, tex_offset,Q);
	    max_dist=0;
		for (j=0; j<n; j++) {
			dist[j]=(float)min((double)dist[j],((double)tex_data[(pivot_count+local_count)*j+local_count+i+tex_offset]==1000.0)?0.0:((double)tex_data[(pivot_count+local_count)*j+local_count+i+tex_offset]));
			if (dist[j]>max_dist) {
				node=j;
				max_dist=dist[j];
			}
		}
	}

	//printf("pivot time = %d\n", clock()-clock1 );

	// get nearest neighbor distance information for each point

	//clock1 = clock();
	nneigbs( graph, n, tex_data, local_count, local_count+pivot_count, tex_offset, pvtmarker, Q );
	//printf("preproc time = %d\n", clock()-clock1 );
	g_preproc_time = clock() - clock1;
	//exit( 0 );

	delete [] dist;
	delete [] pvtmarker;

}
