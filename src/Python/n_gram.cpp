#include "n_gram.h"
#include <iostream>


void 
NGram::addLine(PyObject *index, PyObject *str)
{
	if (!(PyLong_Check(index) && PyUnicode_Check(str))) {
		throw std::string("Wrong type");
	}

	pimpl_.add_line(PyLong_AsUnsignedLongMask(index), str);
}


PyObject *
NGram::search(PyObject *pattern)
{
	return search(pattern, false);
}

PyObject * 
NGram::search(PyObject *pattern, const bool isStrict)
{
	if (!PyUnicode_Check(pattern)) {
		throw std::string("Wrong type");
	}

	PyObject *result = PyList_New(0);
	for (auto &p : pimpl_.search(pattern, isStrict)) {
		PyObject *index = PyLong_FromLong(p.first);
		PyList_Append(result, index);
	}
	return result;
}

void
NGram::delLine(PyObject *index)
{
	if (!PyLong_Check(index)) {
		throw std::string("Wrong type");
	}

	return pimpl_.del_line(PyLong_AsUnsignedLongMask(index));
}

const int 
NGram::size()
{
	return pimpl_.size();
}

const int
NGram::get_c_string(PyObject * str, char* &ref) const {
    Py_ssize_t size;
	ref = PyUnicode_AsUTF8AndSize(str, &size);
	return size;
};

void
NGram::incr_refs(PyObject *str) {
	Py_INCREF(str);
};

void
NGram::decr_refs(PyObject *str) {
	Py_XDECREF(str);
};
