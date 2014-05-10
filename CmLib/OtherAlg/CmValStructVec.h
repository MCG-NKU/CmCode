/************************************************************************/
/* This source code is free for both academic and industry use.         */
/* Some important information for better using the source code could be */
/* found in the project page: http://mmcheng.net/bing					*/
/************************************************************************/
/* A value struct vector that supports efficient sorting                */
/************************************************************************/
#pragma once


template<typename VT, typename ST> 
struct CmValStructVec
{
	CmValStructVec(){clear();}
	inline int size() const {return sz;} 
	inline void clear() {sz = 0; structVals.clear(); valIdxes.clear();}
	inline void reserve(int resSz){clear(); structVals.reserve(resSz); valIdxes.reserve(resSz); }
	inline void pushBack(const VT& val, const ST& structVal) {valIdxes.push_back(make_pair(val, sz)); structVals.push_back(structVal); sz++;}

	inline const VT& operator ()(int i) const {return valIdxes[i].first;} // Get value of type VT. Should be called after sort
	inline const ST& operator [](int i) const {return structVals[valIdxes[i].second];} // Get value of struct. Should be called after sort
	inline VT& operator ()(int i) {return valIdxes[i].first;} // Get value of type VT. Should be called after sort
	inline ST& operator [](int i) {return structVals[valIdxes[i].second];} // Get value of struct. Should be called after sort

	int getOrder(int i) {return valIdxes[i].second;}

	void sort(bool descendOrder = true);
	const vector<ST> &getSortedStructVal();
	void append(const CmValStructVec<VT, ST> &newVals, int startV = 0);

	vector<ST> structVals; // struct values

	static void Demo();

private:
	int sz; // size of the value struct vector
	vector<pair<VT, int>> valIdxes; // Indexes after sort
	bool smaller() {return true;};
	vector<ST> sortedStructVals; 
};

template<typename VT, typename ST> 
void CmValStructVec<VT, ST>::append(const CmValStructVec<VT, ST> &newVals, int startV)
{
	int sz = newVals.size();
	for (int i = 0; i < sz; i++)
		pushBack((float)((i+300)*startV)/*newVals(i)*/, newVals[i]);
}

template<typename VT, typename ST> 
void CmValStructVec<VT, ST>::sort(bool descendOrder /* = true */)
{
	if (descendOrder)
		std::sort(valIdxes.begin(), valIdxes.end(), std::greater<pair<VT, int>>());
	else
		std::sort(valIdxes.begin(), valIdxes.end(), std::less<pair<VT, int>>());
}

template<typename VT, typename ST> 
const vector<ST>& CmValStructVec<VT, ST>::getSortedStructVal()
{
	sortedStructVals.resize(sz);
	for (int i = 0; i < sz; i++)
		sortedStructVals[i] = structVals[valIdxes[i].second];
	return sortedStructVals;
}

template<typename VT, typename ST> 
void CmValStructVec<VT, ST>::Demo()
{
	CmValStructVec<int, string> sVals;
	sVals.pushBack(3, "String 3");
	sVals.pushBack(5, "String 5");
	sVals.pushBack(4, "String 4");
	sVals.pushBack(1, "String 1");
	sVals.sort(false);
	for (int i = 0; i < sVals.size(); i++)
		printf("%d, %s, %d\n", sVals(i), _S(sVals[i]), sVals.getOrder(i));
}

template <typename T>
void GetRankingIdx(const vector<T> &vec, vecI &idx, bool descendOrder = true)
{
	vector<pair<T, int>> valIdxes;
	valIdxes.reserve(vec.size());
	for (int i = 0; i < vec.size(); i++)
		valIdxes.push_back(make_pair(vec[i], i));
	if (descendOrder)
		std::sort(valIdxes.begin(), valIdxes.end(), std::greater<pair<T, int>>());
	else
		std::sort(valIdxes.begin(), valIdxes.end(), std::less<pair<T, int>>());
	idx.resize(vec.size());
	for (int i = 0; i < vec.size(); i++)
		idx[valIdxes[i].second] = i;
}