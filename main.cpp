// INTERFACE /////////////////////////////////////
#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <vector>
#include <utility>

using std::cin;
using std::cerr;
using std::cout;
using std::endl;

/*
 * Мы реализуем стандартный класс для хранения кучи с возможностью доступа
 * к элементам по индексам. Для оповещения внешних объектов о текущих значениях
 * индексов мы используем функцию index_change_observer.
 */

template <class T, class Compare = std::less<T> >
class Heap {
 public:
  using IndexChangeObserver =
      std::function<void(const T& element, size_t new_element_index)>;

  static constexpr size_t kNullIndex = static_cast<size_t>(-1);

  explicit Heap(
      Compare compare = Compare(),
      IndexChangeObserver index_change_observer = IndexChangeObserver());

  size_t push(const T& value);
  void erase(size_t index);
  const T& top() const;
  void pop();
  size_t size() const;
  bool empty() const;

 private:
  IndexChangeObserver index_change_observer_;
  Compare compare_;
  std::vector<T> elements_;

  size_t Parent(size_t index) const;
  size_t LeftSon(size_t index) const;
  size_t RightSon(size_t index) const;

  bool CompareElements(size_t first_index, size_t second_index) const;
  void NotifyIndexChange(const T& element, size_t new_element_index);
  void SwapElements(size_t first_index, size_t second_index);
  size_t SiftUp(size_t index);
  void SiftDown(size_t index);
};


struct MemorySegment {
  int left;
  int right;
  size_t heap_index;

  MemorySegment(int left, int right);
  size_t Size() const;
  MemorySegment Unite(const MemorySegment& other) const;
};

using MemorySegmentIterator = std::list<MemorySegment>::iterator;
using MemorySegmentConstIterator = std::list<MemorySegment>::const_iterator;


struct MemorySegmentSizeCompare {
  bool operator() (MemorySegmentIterator first,
                   MemorySegmentIterator second) const;
};


using MemorySegmentHeap =
    Heap<MemorySegmentIterator, MemorySegmentSizeCompare>;


struct MemorySegmentsHeapObserver {
  void operator() (MemorySegmentIterator segment, size_t new_index) const;
};

/*
 * Мы храним сегменты в виде двухсвязного списка (std::list).
 * Быстрый доступ к самому левому из наидлиннейших свободных отрезков
 * осуществляется с помощью кучи, в которой (во избежание дублирования
 * отрезков в памяти) хранятся итераторы на список — std::list::iterator.
 * Чтобы быстро определять местоположение сегмента в куче для его изменения,
 * мы внутри сегмента в списке храним heap_index, актуальность которого
 * поддерживаем с помощью index_change_observer. Мы не храним отдельной метки
 * для маркировки занятых сегментов: вместо этого мы кладём в heap_index
 * специальный kNullIndex.
 */

class MemoryManager {
 public:
  using Iterator = MemorySegmentIterator;
  using ConstIterator = MemorySegmentConstIterator;

  explicit MemoryManager(size_t memory_size);
  Iterator Allocate(size_t size);
  void Free(Iterator position);
  Iterator end();
  ConstIterator end() const;

 private:
  MemorySegmentHeap free_memory_segments_;
  std::list<MemorySegment> memory_segments_;

  void AppendIfFree(Iterator remaining, Iterator appending);
};


size_t ReadMemorySize(std::istream& stream = std::cin);

struct AllocationQuery {
  size_t allocation_size;
};

struct FreeQuery {
  int allocation_query_index;
};

/*
 * Для хранения запросов используется специальный класс-обёртка
 * MemoryManagerQuery. Фишка данной реализации в том, что мы можем удобно
 * положить в него любой запрос, при этом у нас есть методы, которые позволят
 * гарантированно правильно проинтерпретировать его содержимое. При реализации
 * нужно воспользоваться тем фактом, что dynamic_cast возвращает nullptr
 * при неудачном приведении указателей.
 */

class MemoryManagerQuery {
 public:
  explicit MemoryManagerQuery(AllocationQuery allocation_query);
  explicit MemoryManagerQuery(FreeQuery free_query);

  const AllocationQuery* AsAllocationQuery() const;
  const FreeQuery* AsFreeQuery() const;

 private:
  class AbstractQuery {
   public:
    virtual ~AbstractQuery() {
    }

   protected:
    AbstractQuery() {
    }
  };

  template <typename T>
  struct ConcreteQuery : public AbstractQuery {
    T body;

    explicit ConcreteQuery(T _body)
        : body(std::move(_body)) {
    }
  };

  std::unique_ptr<AbstractQuery> query_;
};

std::vector<MemoryManagerQuery> ReadMemoryManagerQueries(
    std::istream& stream = std::cin);

struct MemoryManagerAllocationResponse {
  bool success;
  size_t position;
};

/*
Мы предоставляем два builder'а - MakeSuccessfulAllocation и
MakeFailedAllocation. Они позволяют создавать корректные объекты
MemoryManagerAllocationResponse и в коде надо пользоваться ими. Но если
захочется как-то по-особенному инициализировать поля структуры (например, для
тестирования), то можно создать структуру с конкретными значениями полей.
*/
MemoryManagerAllocationResponse MakeSuccessfulAllocation(size_t position);

MemoryManagerAllocationResponse MakeFailedAllocation();

std::vector<MemoryManagerAllocationResponse> RunMemoryManager(
    size_t memory_size,
    const std::vector<MemoryManagerQuery>& queries);

void OutputMemoryManagerResponses(
    const std::vector<MemoryManagerAllocationResponse>& responses,
    std::ostream& ostream = std::cout);

int main() {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::istream& input_stream = std::cin;
  std::ostream& output_stream = std::cout;

  const size_t memory_size = ReadMemorySize(input_stream);
  const std::vector<MemoryManagerQuery> queries =
      ReadMemoryManagerQueries(input_stream);

  const std::vector<MemoryManagerAllocationResponse> responses =
      RunMemoryManager(memory_size, queries);

  OutputMemoryManagerResponses(responses, output_stream);

  return 0;
}



























size_t ReadMemorySize(std::istream& stream) {
  size_t memory_size;
  stream >> memory_size;
  return memory_size;
}


std::vector<MemoryManagerQuery> ReadMemoryManagerQueries(
    std::istream& stream) {
  unsigned queries_number;
  stream >> queries_number;
  std::vector<MemoryManagerQuery> queries;
  for (auto query_n = 0U; query_n < queries_number; ++query_n) {
    int query_numeric;
    stream >> query_numeric;
    if (query_numeric >= 0) {
      AllocationQuery allocation_query = {static_cast<size_t>(query_numeric)};
      queries.push_back(MemoryManagerQuery(allocation_query));
    } else {
      FreeQuery free_query = {-query_numeric - 1};
      queries.push_back(MemoryManagerQuery(free_query));
    }
  }

  return queries;
}


/** MemoryManagerQuery: BEGIN **/
MemoryManagerQuery::MemoryManagerQuery(AllocationQuery allocation_query)
  : query_(new ConcreteQuery<AllocationQuery>(allocation_query))
{ }


MemoryManagerQuery::MemoryManagerQuery(FreeQuery free_query)
  : query_(new ConcreteQuery<FreeQuery>(free_query))
{ }


const AllocationQuery* MemoryManagerQuery::AsAllocationQuery() const {
  auto query = dynamic_cast<ConcreteQuery<AllocationQuery>*>(query_.get());
  if (query) {
    return &query->body;
  }
  return nullptr;
}


const FreeQuery* MemoryManagerQuery::AsFreeQuery() const {
  auto query = dynamic_cast<ConcreteQuery<FreeQuery>*>(query_.get());
  if (query) {
    return &query->body;
  }
  return nullptr;
}


/** MemoryManagerQuery: END **/


std::vector<MemoryManagerAllocationResponse> RunMemoryManager(
    size_t memory_size,
    const std::vector<MemoryManagerQuery>& queries) {
  MemoryManager memory_manager(memory_size);
  std::vector<MemoryManager::Iterator> results(queries.size());
  std::vector<MemoryManagerAllocationResponse> responses;
  for (auto query_n = 0U; query_n < queries.size(); ++query_n) {
    const auto& query = queries[query_n];
    if (auto query_pointer = query.AsAllocationQuery()) {
      auto result = memory_manager.Allocate(query_pointer->allocation_size);
      results[query_n] = result;
      if (result._M_node) {
        responses.push_back(MakeSuccessfulAllocation(result->left));
      } else {
        responses.push_back(MakeFailedAllocation());
      }
    } else if (auto query_pointer = query.AsFreeQuery()) {
      results[query_n] = MemoryManager::Iterator(nullptr);
      auto result = results[query_pointer->allocation_query_index];
      if (result._M_node) {
        memory_manager.Free(result);
      }
    } else {
      throw std::logic_error("Unknown Memory Manager query!");
    }
  }
  return responses;
}


MemoryManagerAllocationResponse MakeSuccessfulAllocation(size_t position) {
  MemoryManagerAllocationResponse response = {true, position};
  return response;
}


MemoryManagerAllocationResponse MakeFailedAllocation() {
  MemoryManagerAllocationResponse response = {false, 0};
  return response;
}


void OutputMemoryManagerResponses(
    const std::vector<MemoryManagerAllocationResponse>& responses,
    std::ostream& ostream) {
  for (auto& response : responses) {
    if (response.success) {
      ostream << response.position + 1 << endl;
    } else {
      ostream << -1 << endl;
    }
  }
}


/** MemoryManager: BEGIN **/
MemoryManager::MemoryManager(size_t memory_size)
  : free_memory_segments_(MemorySegmentHeap(MemorySegmentSizeCompare(),
                                            MemorySegmentsHeapObserver()))
  , memory_segments_(std::list<MemorySegment>())
{
  MemorySegment initial_memory(0, memory_size);
  auto memory_segment_iterator =
      memory_segments_.insert(memory_segments_.end(), initial_memory);
  free_memory_segments_.push(memory_segment_iterator);
}


MemoryManager::Iterator MemoryManager::Allocate(size_t size) {
  if (free_memory_segments_.size() == 0) {
    return Iterator(nullptr);
  }
  auto max_free_memory_segment_iterator = free_memory_segments_.top();
  if (size > max_free_memory_segment_iterator->Size()) {
    return Iterator(nullptr);
  }
  if (size == max_free_memory_segment_iterator->Size()) {
    free_memory_segments_.erase(max_free_memory_segment_iterator->heap_index);
    return max_free_memory_segment_iterator;
  }
  auto allocated_memory_iterator =
      memory_segments_.insert(
        max_free_memory_segment_iterator,
        MemorySegment(max_free_memory_segment_iterator->left,
                      max_free_memory_segment_iterator->left + size));
  max_free_memory_segment_iterator->left = allocated_memory_iterator->right;
  free_memory_segments_.erase(max_free_memory_segment_iterator->heap_index);
  free_memory_segments_.push(max_free_memory_segment_iterator);
  return allocated_memory_iterator;
}


void MemoryManager::Free(Iterator position) {
  auto left_iterator = position;
  --left_iterator;
  auto right_iterator = position;
  ++right_iterator;
  if (position != memory_segments_.begin()) {
    AppendIfFree(position, left_iterator);
  }
  if (right_iterator != memory_segments_.end()) {
    AppendIfFree(position, right_iterator);
  }
  free_memory_segments_.push(position);
}


MemoryManager::Iterator MemoryManager::end() {
  return memory_segments_.end();
}


MemoryManager::ConstIterator MemoryManager::end() const {
  return memory_segments_.cend();
}


void MemoryManager::AppendIfFree(Iterator remaining, Iterator appending) {
  if (appending->heap_index != MemorySegmentHeap::kNullIndex) {
    *remaining = remaining->Unite(*appending);
    free_memory_segments_.erase(appending->heap_index);
    memory_segments_.erase(appending);
  }
}


/** MemoryManager: END **/


void MemorySegmentsHeapObserver::operator() (
    MemorySegmentIterator segment, size_t new_index) const {
  segment->heap_index = new_index;
}


bool MemorySegmentSizeCompare::operator() (
    MemorySegmentIterator first, MemorySegmentIterator second) const {
  if (first->Size() == second->Size()) {
    return first->left < second->left;
  }
  return first->Size() > second->Size();
}


/** MemorySegment: BEGIN **/
MemorySegment::MemorySegment(int left, int right)
  : left(left), right(right), heap_index(MemorySegmentHeap::kNullIndex)
{ }


size_t MemorySegment::Size() const {
  return right >= left ? right - left : 0;
}


MemorySegment MemorySegment::Unite(const MemorySegment& other) const {
  if (left == other.right) {
    return MemorySegment(other.left, right);
  } else if (right == other.left) {
    return MemorySegment(left, other.right);
  } else {
    throw std::runtime_error("Memory Segments to Unite are not adjacent!");
  }
}


/** MemorySegment: END **/


/** Heap: BEGIN **/
template <class T, class Compare>
Heap<T, Compare>::Heap(
    Compare compare, IndexChangeObserver index_change_observer)
  : index_change_observer_(index_change_observer)
  , compare_(compare)
  , elements_(std::vector<T>())
{ }


template <class T, class Compare>
size_t Heap<T, Compare>::push(const T& value) {
  elements_.push_back(value);
  NotifyIndexChange(value, this->size() - 1);
  return SiftUp(this->size() - 1);
}


template <class T, class Compare>
void Heap<T, Compare>::erase(size_t index) {
  SwapElements(index, this->size() - 1);
  NotifyIndexChange(elements_.back(), kNullIndex);
  elements_.pop_back();
  SiftUp(index);
  SiftDown(index);
}


template <class T, class Compare>
const T& Heap<T, Compare>::top() const {
    return elements_[0];
}


template <class T, class Compare>
void Heap<T, Compare>::pop() {
  SwapElements(0, this->size() - 1);
  NotifyIndexChange(elements_.back(), kNullIndex);
  elements_.pop_back();
  SiftDown(0);
}


template <class T, class Compare>
size_t Heap<T, Compare>::size() const {
  return elements_.size();
}


template <class T, class Compare>
bool Heap<T, Compare>::empty() const {
  return elements_.empty();
}


template <class T, class Compare>
size_t Heap<T, Compare>::Parent(size_t index) const {
  return index ? (index - 1) / 2 : index;
}


template <class T, class Compare>
size_t Heap<T, Compare>::LeftSon(size_t index) const {
  return index < this->size() / 2 ?
        2 * index + 1 :
        kNullIndex;
}


template <class T, class Compare>
size_t Heap<T, Compare>::RightSon(size_t index) const {
  return this->size() > 2 && index < (this->size() - 1) / 2 ?
        2 * index + 2 :
        kNullIndex;
}


template <class T, class Compare>
bool Heap<T, Compare>::CompareElements(
    size_t first_index, size_t second_index) const {
  return compare_(elements_[first_index], elements_[second_index]);
}


template <class T, class Compare>
void Heap<T, Compare>::NotifyIndexChange(
    const T& element, size_t new_element_index) {
  index_change_observer_(element, new_element_index);
}


template <class T, class Compare>
void Heap<T, Compare>::SwapElements(size_t first_index, size_t second_index) {
  NotifyIndexChange(elements_[first_index], second_index);
  NotifyIndexChange(elements_[second_index], first_index);
  std::swap(elements_[first_index], elements_[second_index]);
}


template <class T, class Compare>
size_t Heap<T, Compare>::SiftUp(size_t index) {
  auto new_index = index;
  while (index != 0) {
    if (CompareElements(index, Parent(index))) {
      SwapElements(index, Parent(index));
      index = Parent(index);
    } else {
      new_index = index;
      index = 0;
    }
  }
  return new_index;
}


template <class T, class Compare>
void Heap<T, Compare>::SiftDown(size_t index) {
  while (LeftSon(index) != kNullIndex) {
    auto best_son_index = LeftSon(index);
    if (RightSon(index) != kNullIndex &&
        CompareElements(RightSon(index), LeftSon(index))) {
      best_son_index = RightSon(index);
    }
    if (CompareElements(best_son_index, index)) {
      SwapElements(best_son_index, index);
      index = best_son_index;
    } else {
      index = kNullIndex;
    }
  }
}


/** Heap: END **/
