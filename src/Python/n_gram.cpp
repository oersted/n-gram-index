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
	return search(pattern, 0);
}

PyObject *
NGram::search(PyObject *pattern, const unsigned int max_edit_dist)
{
	if (!PyUnicode_Check(pattern)) {
		throw std::string("Wrong type");
	}

	PyObject *result = PyList_New(0);
	for (auto &p : pimpl_.search(pattern, max_edit_dist)) {
		PyObject *index = PyLong_FromLong(p.first);
		PyObject *ed = PyLong_FromLong(p.second);
		PyObject *tup = PyTuple_New(2);
		PyTuple_SetItem(tup, 0, index);
		PyTuple_SetItem(tup, 1, ed);
		PyList_Append(result, tup);
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
