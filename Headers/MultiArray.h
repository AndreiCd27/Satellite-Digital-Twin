#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>


struct Handle {
	int offset = 0;
	int size = 0;
	int capacity = 0;
	int id = -1;
};

class ArrayOrganizerException : public std::runtime_error {
public:
	ArrayOrganizerException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

template <typename T>
class ArrayOrganizer {

	int contor = 0;

	std::unordered_map<int, int> ID_TO_INDEX;

	std::vector<int> INDEX_TO_ID;

	std::vector<Handle> Handles;

	std::vector<T> Array;

	void DoubleInSize(int HandleIndex) {
		Handle& h = Handles[HandleIndex];
		int capacityOld = h.capacity;

		int arraySizeOld = Array.size();
		Array.resize(arraySizeOld + capacityOld);

		// std::copy_backward
		// Copy in reverse to avoid overwritten data
		// Here arraySize is the size of the vector before resizing
		// EX: double h1
		// | t0 | t1 | t1 | t2 | t3 | t3 |
		//      ^         ^
		// | t0 | t1 | t1 | t2 | t3 | t3 |    |    |			+ 2 spots
		//                ^
		// | t0 | t1 | t1 | t2 | t3 | t3 |    | t3 |			copies first template T with Handle = 3
		// | t0 | t1 | t1 | t2 | t3 |    | t3 | t3 |			copies second template T with Handle = 3
		// | t0 | t1 | t1 | t2 | t3 | t2 | t3 | t3 |			copies first template T with Handle = 2
		// | t0 | t1 | t1 | t2 | t3 | t2 | t3 | t3 |			data from index 3-4 can be OVERWRITTEN now
		// Prevent unwanted behaviour by overwriteing data from index 3-4 with EMPTY template
		auto from = Array.begin() + h.offset + capacityOld;
		auto to = Array.begin() + arraySizeOld;
		std::copy_backward(from, to, Array.end());

		for (int i = HandleIndex + 1; i < Handles.size(); i++) {
			Handles[i].offset += capacityOld;
		}
		h.capacity <<= 1;

		std::fill(Array.begin() + h.offset + capacityOld, Array.begin() + h.offset + h.capacity, T());

	}

	void ReservePow2(int HandleIndex, int pow) {
		Handle& h = Handles[HandleIndex];
		int capacityOld = h.capacity;
		int capacityNew = h.capacity << pow;

		int arraySizeOld = Array.size();
		Array.resize(arraySizeOld + capacityNew - capacityOld);

		auto from = Array.begin() + h.offset + capacityOld;
		auto to = Array.begin() + arraySizeOld;
		std::copy_backward(from, to, Array.end());

		for (int i = HandleIndex + 1; i < Handles.size(); i++) {
			Handles[i].offset += (capacityNew - capacityOld);
		}
		h.capacity = capacityNew;

		std::fill(Array.begin() + h.offset + capacityOld, Array.begin() + h.offset + h.capacity, T());

	}

public:

	void NewHandle(int HandleID, int capacityPow2) {
		// Capacity SHOULD be a power of 2 for correct memory alignment,
		// But it is not enforced

		int h_off = (int) Array.size();

		Handle h = { h_off , 0, capacityPow2, HandleID };
		Handles.push_back(h);

		Array.resize(h_off + capacityPow2);

		ID_TO_INDEX[HandleID] = Handles.size() - 1;
		INDEX_TO_ID.push_back(HandleID);

		std::cout << "Handle: " << HandleID << " Offset: " << h.offset << "\n";
	}

	void MoveBefore(int beforeHandleID, int HandleID) {
		//TODO
	}
	void MoveAfter(int afterHandleID, int HandleID) {
		//TODO
	}
	void MoveToEnd(int HandleID) {

		int HandleIndex = ID_TO_INDEX[HandleID]; // (OLD INDEX)
		if (HandleIndex == Handles.size() - 1) return;
		Handle& h = Handles[HandleIndex];

		std::vector<T> temp;
		temp.resize(h.capacity);
		auto from = Array.begin() + h.offset;
		auto to = from + h.capacity;
		std::copy(from, to, temp.begin());

		std::copy(to, Array.end(), from);

		std::copy(temp.begin(), temp.end(), Array.end() - h.capacity);

		for (int i = HandleIndex + 1; i < Handles.size(); i++) {
			Handles[i].offset -= h.capacity;
			int iID = INDEX_TO_ID[i];
			ID_TO_INDEX[iID] = i - 1;
		}

		Handle hcopy = h; // We copy h to prevent erase deleting our handle
		hcopy.offset = Array.size() - hcopy.capacity;

		Handles.erase(Handles.begin() + HandleIndex);
		INDEX_TO_ID.erase(INDEX_TO_ID.begin() + HandleIndex);
		Handles.push_back(hcopy);
		INDEX_TO_ID.push_back(HandleID);

		ID_TO_INDEX[HandleID] = Handles.size() - 1;
	}
	void MoveToStart(int HandleID) {
		//TODO
	}

	void Push(int HandleID, T data) {

		int HandleIndex = ID_TO_INDEX[HandleID];

		Handle& h = Handles[HandleIndex];

		if (h.size >= h.capacity) {
			DoubleInSize(HandleIndex);
		}

		Array[h.offset + h.size] = data;
		h.size++;
	}
	
	void PushMultipleData(int HandleID, std::vector<T>& multidata) {
		int HandleIndex = ID_TO_INDEX[HandleID];

		Handle h = Handles[HandleIndex];

		int oldSize = Array.size();

		if ( h.size + multidata.size() > h.capacity ) {
			int capacityTarget = h.capacity;
			if (capacityTarget == 0) capacityTarget = multidata.size();
			while (h.size + multidata.size() > capacityTarget) {
				capacityTarget <<= 1;
				
			}

			int incrrease = capacityTarget - h.capacity;
			Array.resize(oldSize + incrrease);

			if (HandleIndex < Handles.size() - 1) {
				auto from = Array.begin() + h.offset + h.capacity;
				auto to = Array.begin() + oldSize;
				std::copy_backward(from, to, Array.end());

				for (int i = HandleIndex + 1; i < Handles.size(); i++) {
					Handles[i].offset += incrrease;
				}
			}
			h.capacity = capacityTarget;
			Handles[HandleIndex].capacity = capacityTarget;
		}

		std::copy(multidata.begin(), multidata.end(), Array.begin() + Handles[HandleIndex].offset + h.size);
		Handles[HandleIndex].size += multidata.size();
		std::cout << " [MultiArray.h PushMultipleData()]\n Pushed " << multidata.size() << " elements into Array (OLD SIZE: " << oldSize << ") NEW SIZE: " << Array.size()<<"\n";
	}
	
	void print() {
		int i = 0;
		for (Handle& h : Handles) {
			std::cout << i << ": ID = " << INDEX_TO_ID[i] << ", OFFSET = " << h.offset << ", SIZE = " << h.size << ", CAPACITY = " << h.capacity << "\n";
			i++;
		}
	}

	Handle GetHandleData(int HandleID) {
		auto iterator = ID_TO_INDEX.find(HandleID);
		if (iterator == ID_TO_INDEX.end()) {
			throw ArrayOrganizerException("Invalid ID " + std::to_string(HandleID), 0);
		}
		return Handles[iterator->second]; // Avoid another look-up in the unordered map
	}

	const T* GetPointerFromHandle(int HandleID) {
		auto iterator = ID_TO_INDEX.find(HandleID);
		if (iterator == ID_TO_INDEX.end()) {
			throw ArrayOrganizerException("Invalid ID " + std::to_string(HandleID), 0);
		}
		int i = Handles[ID_TO_INDEX[HandleID]].offset;
		return &Array[i];
	}

	std::vector<T>& GetMultiArray() {
		return Array;
	}

	bool ContainsHandle(int HandleID) {
		return !(ID_TO_INDEX.find(HandleID) == ID_TO_INDEX.end());
	}
	/*
	const int GetSizeBetweenHandles(int HandleID_0, int HandleID_1) {
		int I_START = ID_TO_INDEX[HandleID_0];
		int I_END = ID_TO_INDEX[HandleID_1];
		int SIZE = Handles[I_END].offset - Handles[I_START].offset + Handles[I_END].size;
		if (SIZE < 0) {
			throw ArrayOrganizerException("Handles given are not in order", 1);
		}
		return SIZE;
	}
	*/

	std::vector<Handle>& GetAllHandles() {
		return Handles;
	}
};