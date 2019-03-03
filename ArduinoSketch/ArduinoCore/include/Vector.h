#pragma once

template <class T>
class ListNode {
	public:
	T element;
	ListNode* next;
	ListNode* prev;

	ListNode(T element, ListNode* prev, ListNode* next) : element(element)
	{
		this->next = next;
		this->prev = prev;
	};
};

template <class T>
class LinkedList  {
	private:
	int _length;
	ListNode<T>* _head;
	ListNode<T>* _tail;
	ListNode<T>* _curr;
	public:
	LinkedList();
	LinkedList(const LinkedList<T>&);
	~LinkedList();
	T& current();
	T& first() const;
	T& last() const;
	int length();
	void append(T);
	void deleteLast();
	void deleteFirst();
	void deleteCurrent();
	bool next();
	bool moveToStart();
	bool prev();
	void remove(T&);
	bool search(T);
	void clear();
	void putFirstToLast();
	void update(T elem);
	LinkedList& operator = (const LinkedList<T>&);
};

template <class T>
LinkedList<T>::LinkedList() {
	_length = 0;
	_head = nullptr;
	_tail = nullptr;
	_curr = nullptr;
}

template <class T>
LinkedList<T>::LinkedList(const LinkedList<T> & list) {
	_length = 0;
	_head = nullptr;
	_tail = nullptr;
	_curr = nullptr;

	ListNode<T> * temp = list._head;

	while(temp != nullptr)
	{
		append(temp->element);
		temp = temp->next;
	}
}

template <class T>
LinkedList<T> & LinkedList<T>::operator=(const LinkedList<T> & list)
{
	clear();

	ListNode<T> * temp = list._head;

	while(temp != nullptr)
	{
		append(temp->element);
		temp = temp->next;
	}

	return *this;
}

template <class T>
LinkedList<T>::~LinkedList() {
	clear();
}

template<class T>
T& LinkedList<T>::current()
{
	return _curr->element;
}

template<class T>
T& LinkedList<T>::first() const
{
	return _head->element;
}

template<class T>
T& LinkedList<T>::last() const
{
	return _tail->element;
}

template<class T>
int LinkedList<T>::length()
{
	return _length;
}

template <class T>
void LinkedList<T>::append(T element)
{
	ListNode<T> * node = new ListNode<T>(element, _tail, nullptr);

	if(_length == 0)
	_curr = _tail = _head = node;
	else {
		_tail->next = node;
		_tail = node;
	}

	_length++;

}

template <class T>
void LinkedList<T>::deleteLast(){
	if(_length == 0)
		return;
	_curr = _tail;
	deleteCurrent();
}

template <class T>
void LinkedList<T>::deleteFirst(){
	if(_length == 0)
		return;
	_curr = _head;
	deleteCurrent();
}

template <class T>
bool LinkedList<T>::next(){
	if(_length == 0)
		return false;

	if(_curr->next == nullptr)
		return false;

	_curr = _curr->next;
	return true;
}

template <class T>
bool LinkedList<T>::moveToStart(){
	_curr = _head;
	return _length != 0;
}

template<class T>
bool LinkedList<T>::prev(){
	if(_length == 0)
		return false;

	if(_curr->prev != nullptr)
		return false;

	_curr = _curr->prev;
	return true;
}

template <class T>
void LinkedList<T>::remove(T & elem){
	if(search(elem))
		deleteCurrent();
}

template <class T>
void LinkedList<T>::deleteCurrent(){
	if(_length == 0)
		return;
	_length--;
	ListNode<T> * temp = _curr;

	if(temp->prev != nullptr)
		temp->prev->next = temp->next;
	if(temp->next != nullptr)
		temp->next->prev = temp->prev;

	if(_length == 0)
		_head = _curr = _tail = nullptr;
	else if(_curr == _head)
		_curr = _head = _head->next;
	else if(_curr == _tail)
		_curr = _tail = _tail->prev;
	else
		_curr = _curr->prev;

	delete temp;
}

template <class T>
bool LinkedList<T>::search(T elem){
	if(_length == 0)
		return false;
	if(moveToStart())
	do {
		if(_curr->element == elem)
		return true;
	} while (next());
	return false;
}

template <class T>
void LinkedList<T>::putFirstToLast(){
	if(_length < 2)
		return;
	ListNode<T>* temp = _head->next;
	_head->next->prev = nullptr;
	_head->next = nullptr;
	_head->prev = _tail;
	_tail->next = _head;
	_tail = _head;
	_head = temp;
}

template <class T>
void LinkedList<T>::update(T elem)
{
	if(Search(elem))
	_curr->element = elem;
}

template <class T>
void LinkedList<T>::clear()
{
	if(_length == 0)
	return;
	ListNode<T> * temp = _head;

	while(temp != nullptr)
	{
		_head = _head->next;
		delete temp;
		temp = _head;
	}

	_head = _curr = _tail = nullptr;

}








