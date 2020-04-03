#include <iostream> 
#include <thread> 
#include <time.h> 
#include <algorithm>
#include <vector>

// merge function for merging two parts
// beg, mid and end - iterators from one container
template <typename Iterator>
void merge(Iterator beg, Iterator mid, Iterator end) 
{ 
	// n1 is size of left part and n2 is size of right part 
	int n1 = mid - beg + 1;
	int n2 = end - mid;

	std::vector<typename std::iterator_traits<Iterator>::value_type> left(n1); 
	std::vector<typename std::iterator_traits<Iterator>::value_type> right(n2); 

	std::move(beg, mid + 1, left.begin());
	std::move(mid + 1, end + 1, right.begin());

	std::merge(left.begin(), left.begin() + n1, right.begin(), right.begin() + n2, beg);

} 

// thread function for multi-threading 
template <typename Iterator>
void merge_sort(Iterator begin, Iterator end) 
{ 
	Iterator mid = begin + (end - begin) / 2; 

	if (begin < end) { 
		merge_sort(begin, mid); 
		merge_sort(mid + 1, end); 
		merge(begin, mid, end); 
	} 
} 

// entry point for multi-threading. Result will be placed into it
template <typename Iterator>
void parallel_sort(Iterator it, size_t n_elems, size_t n_threads) {

	std::thread threads[n_threads]; 
	size_t part_len = (n_elems - 1) / n_threads + 1;

	// creating threads 
	for (size_t i = 0; i < n_threads; i++) 
		threads[i] = std::thread(merge_sort<Iterator>,
					   it+ i * part_len,
					   it + std::min((i + 1) * part_len - 1, n_elems - 1)
					 );

	for (auto&& thread : threads) 
		thread.join();

	//merging final parts
	for (size_t i = 1; i < n_threads; i++) {
		merge(it, it + i * part_len - 1, it + std::min((i + 1) * part_len - 1, n_elems - 1));
	}
}

// Driver Code 
int main() 
{ 
	const int amount = 22;
	const int th_amount = 4; 
	std::srand(std::time(nullptr));
	
	std::vector<int> a(amount);

	// generating random values in array 
	std::cout << "Input array:  "; 
	for (int i = 0; i < amount; i++) {
		a[i] = rand() % 100; 
		std::cout << a[i] << " "; 
	}
	std::cout << std::endl;

	parallel_sort(a.begin(), amount, th_amount);

	// displaying sorted array 
	std::cout << "Sorted array: "; 
	for (int i = 0; i < amount; i++) 
		std::cout << a[i] << " "; 
	std::cout << std::endl;

	return 0; 
}
