#ifndef EMBEDDERDEFS_H
#define EMBEDDERDEFS_H

struct vtx_data {
	int match;	// used for graph coarsening
	int nedges;
  int *edges;
  float *ewgts;
};

typedef int DistType; // must be signed!!

inline double max(double x, double y) {
        if (x>=y)
                return x;
        else
                return y;
}

inline double min(double x, double y) {
        if (x<=y)
                return x;
        else
                return y;
}

inline int max(int x, int y) {
        if (x>=y)
                return x;
        else
                return y;
}

inline int min(int x, int y) {
        if (x<=y)
                return x;
        else
                return y;
}

struct Point {
        double x;
        double y;
        int operator==(Point other) { return x==other.x && y==other.y; }
};

class Queue {
 private:       
  int * data;
  int queueSize;
  int end;
  int start;
 public:
  Queue(int size) {data=new int[size]; queueSize=size; start=0; end=0;}
  ~Queue() {delete [] data;}

  void initQueue(int startVertex) {
    data[0]=startVertex;
    start=0; end=1;
  }

  bool dequeue(int & vertex) {          
    if (start>=end)
      return false; // underflow
    vertex=data[start++];
    return true;
  }

  bool enqueue(int vertex) {
    if (end>=queueSize)
      return false; // overflow
    data[end++]=vertex;
    return true;
  }
};

void bfs(int vertex, vtx_data * localGraph, int n, float * dist, int width, int offset, int m_offset, Queue & Q);
vtx_data *load_vtx_data( const char *filename, int *n );
void extract_distances( vtx_data * graph, int n, int local_count, int pivot_count, float *tex_data, int tex_offset );
#endif