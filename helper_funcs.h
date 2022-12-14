inline void shif_left_insert (float shifted_array[], int size, float insert)
/*
Inserts a value in an array, and shifts all current values in the array to the left.]
To insert the value 10 in the array a = [1,2,3], the reult is a = [2,3,10].
*/
{
    if(size == 1){
        //do nothing
    }
    else{
        float temp;
        for (int i=0; i < size; i++) {
            shifted_array[i] = shifted_array[i + 1];//myarray[0] == myarray[1]
        }
    }
    shifted_array[size-1] = insert;
}

inline int map(float val, float min_range_1, float max_range_1, float min_range_2, float max_range_2) { 
/*
Maps a value within a range to a different range. Custom implementation of the arduino map function.
To map the value of 50 in the range 0-100, in the range of 800-1000 the result will be 900.
*/
               float a = (val - min_range_1) / (max_range_1 - min_range_1);
               float map = min_range_2 + a * (max_range_2 - min_range_2);
            return map;
           }

inline float array_average(float a[], int size) {
/*
Takes the average of an array of floats
*/
    float summer;
    for (int i=0; i < size; i++) {
        summer += a[i];
    }
    return summer/size;
    // return (summer / size_array);
}


inline int compareArrays(int a[], int b[], int n) {
/*
Compares two arrays of equal size(n). Returns a 1 if arrays are equal, otherwise returns 0
*/
  int ii;
  for(ii = 0; ii <= n; ii++) {
    if (a[ii] != b[ii]) return 0;
  }
  return 1;
}

