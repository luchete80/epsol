/************************************************************************

	Copyright 2012-2013 Luciano Buglioni

	Contact: luciano.buglioni@gmail.com

	This file is a part of FluxSol

	FluxSol is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    Free CFD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a copy of the GNU General Public License,
    see <http://www.gnu.org/licenses/>.

*************************************************************************/
#ifndef _MATRIX_H
#define _MATRIX_H

#include <vector>
#include <fstream>
#include <cstdarg>
#include <iostream>

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::ostringstream
#include <iomanip>      // std::setprecision

#include "../Type/Tensor.h"
#include "../Type/pTraits.h"
#include "../Type/Products.h"



using namespace std;

namespace FluxSol
{

	class MatrixIndex
	{

	protected:
		int rows, cols;
	public:
		MatrixIndex(){}
		MatrixIndex(const int &r, const int &c) :rows(r), cols(c){}
		const int & Rows()const{ return rows; }
		const int & Cols()const{ return cols; }
	};
//Matrix of any dimension
template<typename T>
class Matrix://:public Tensor
	public MatrixIndex
{
	private:

		std::vector< std::vector<T> > v_;	//Like Phisical Tensor components


    protected:
        T determ;

	public:

	//To access with [][]

	class CRow {
		friend class Matrix;
	public:
		inline T& operator[](int col)
		{
			return parent.v_[row][col];
		}
	private:
		CRow(Matrix &parent_, int row_) :
			parent(parent_),
			row(row_)
		{}

		Matrix& parent;
		int row;
	};

	//NOT CONST, CAN BE USED AS LVALUE
	//inline const CRow operator[](int row)const
	inline CRow operator[](int row)
	{
		return CRow(*this, row);
	}


	enum
    {
        rank = 2 // Rank of Vector is 1, esto sirve para las templates
    };


		//Constructors
		Matrix(){}
		//Con arg...
		//From components
		//inline Matrix(int r, int c);
		inline explicit Matrix
			(int rowlen,int collen,...);


		//operations
		inline Matrix<T> operator*(const Matrix<T> &t2) const ;
		inline const Matrix<T> operator+(const Matrix<T> &t2) const ;
		inline const Matrix<T> operator-(const Matrix<T> &t2) const ;
		inline const Matrix<T> operator*(const T &d) const;

		inline Matrix<T> & operator=(const T &num)
		{
            for (int r=0;r<this->rows;r++)
                for (int s=0;s<this->cols;s++)
                {
                    v_[r][s]=num;
                }

            return *this;
		}

		inline Matrix<T> & operator=(const Matrix<T> &m)
		{
			this->v_.clear();

			this->rows = m.Rows();
			this->cols = m.Cols();
			vector<T> temp;
			for (int i = 0; i < rows; i++)
			{
				temp.clear();
				for (int j = 0; j < cols; j++)
					temp.push_back(m[i][j]);

				v_.push_back(temp);

			}
			this->determ=m.determ;
			return *this;
		}



		//RVALUEv_
		const double & Val(const int &s, const int &t)const{ return v_[s][t]; }

		const std::string outstr()const;

		//how to overload this
		//CONST VAL
		inline const T & cval(const int &r, const int &c)const{ return this->v_[r][c]; }


		//Slow function
		inline Matrix<T> inv()const;
		inline T det()const;
		inline const T & det_()const{return this->determ;}   //Do not calculate it

		void SetIdentity();

		//Transpose
		inline Matrix<T> Tr()const;
		void Clear();
		void Set_Determinant();

};

class OrderedPair
{
	int row, col;

	public:
		OrderedPair(){}
		inline OrderedPair(const int &r, const int &c) :row(r), col(c){};

		std::vector<int> & Val(){
			vector<int>v; v.push_back(row); v.push_back(col);
			return v;
		}

		inline const int Row()const{ return row; }
		inline const int Col()const{ return col; }

		inline void Set(const unsigned int &r, const unsigned int &c){ row = r; col = c; }


};

//SPARSE MATRIX INDEX
class SparseMatrixIndex
{


protected:
	std::vector <OrderedPair> pos;
	const int rows, cols;



public:
	//SparseMatrix(){}
	SparseMatrixIndex():rows(0),cols(0){}
	explicit SparseMatrixIndex(const int &r, const int & c) :rows(r), cols(c){}

	inline const OrderedPair & Pair(const unsigned int p)const{ return pos[p]; }
	//NUMBER OF TOTAL VALUES, FOLLOWED BY UTIL VALUES

	//ACCESS GAUSS

};


//Used By elements
//GENERIC, ONLY BULK DATA
//SINGLE SParse MaTRIX; THISMATRIX DOES NOT CONTAIN ROWS,COLS AND INDEX
class SparseMatrixBulk
{


protected:
	std::vector <double> val;



public:
	//SparseMatrix(){}
	SparseMatrixBulk(){}
	//NUMBER OF TOTAL VALUES, FOLLOWED BY UTIL VALUES
	inline SparseMatrixBulk(const int &numvals){
		for (int v = 0; v < numvals; v++)
			val.push_back(0.);
		}



	inline Matrix<double> Mat()const;	//Returns for operations

	inline double & operator[](const int &i){ return val[i]; }

	inline const double & Val(const int &i)const{ return val[i]; }

	inline double operator=(const vector<double> &v);



	//ACCESS GAUSS

};



//Used By elements
//GENERIC, ONLY BULK DATA
//SINGLE SParse MaTRIX; THISMATRIX
class SparseMatrix:
	public SparseMatrixBulk,
	public SparseMatrixIndex
{


protected:
	std::vector <double> val;
	std::vector <OrderedPair> pos;
	const int rows, cols;



public:
	//SparseMatrix(){}
	SparseMatrix() :rows(0), cols(0){}
	inline explicit SparseMatrix(const int &r, const int & c) :rows(r), cols(c){};
	//NUMBER OF TOTAL VALUES, FOLLOWED BY UTIL VALUES
	inline SparseMatrix(const int &r, const int & c, const int &numvals) :rows(r), cols(c){
		for (int v = 0; v < numvals; v++)
		{
			this->pos.push_back(OrderedPair());
			//this is temporary
			val.push_back(0.);

		}

	}

	//WHEN UTIL VAL IS USED
	inline SparseMatrix(const int &r, const int & c, const int &numvals, const int &utilvals):
		SparseMatrix(r, c, numvals)
		{}

	inline Matrix<double> Mat()const;	//Returns for operations

	inline double & operator[](const int &i){ return val[i]; }

	inline double operator=(const vector<double> &v);



	//ACCESS GAUSS

};


//Original in OpenFOam
//template<class Cmpt>
//class typeOfRank<Cmpt, 2>
//template<typename T>
//class typeOfRank<2>
//{
//	public:
//	typedef Matrix <T> type;
//
//};


//Inline
//Constructors


//- Construct from components
template <typename T>
inline Matrix<T>::Matrix
(int rowlen, int collen,...)
:MatrixIndex(rowlen,collen)
{
	va_list vl;
	int numelem = rowlen*collen;

	va_start(vl, collen);
	//v_.push_back(va_arg(vl, T));
	vector<T> temp;
	for (int i = 0; i < rows; ++i)
	{
		temp.clear();
		for (int j = 0; j < cols; j++)
		{
			temp.push_back(va_arg(vl, T));
		}
		for (int j = 0; j < cols; j++)
			temp[j] = 0.;

		v_.push_back(temp);

	}
	va_end(vl);
}


//#include "Matrix_Def.h"



// DIFFERENT MATRIX TYPE



	//ThESE aRE not REALLY matriceS, but returns them



	//THIS CLASS INCLUDES EVERY FIELD, IS THE GLOBAL MATRIX
	//INHERITS FROM SQ AND SYMMETRIC
	class SqSparseSymmMatrix
		:public SparseMatrix
	{

	protected:

	public:


	};


	//H a.k.a. N matrix
	//is not sparse
	//IMPORTANT
	//is not necessary to repeat this matrix in all elements because is the same for each element type
	//TEMPLATE INDICATES WhICH VARIABLE INTERPOLATES (SCALAR, VEC2D OR VEC3D)



	template <typename T>
	Matrix<T> Matrix<T>::operator*(const Matrix<T> &t2) const
	{

		Matrix<T> m(this->rows, t2.cols);

		if (this->cols == t2.rows)
		{
			for (int f = 0; f < this->rows; f++)
			{
				for (int cext = 0; cext < t2.cols; cext++)
				{
					T val = 0.0;
					for (int c = 0; c < this->cols; c++)
						val += this->v_[f][c] * t2.v_[c][cext];

					m[f][cext] = val;
				}

			}
		}

		return m;

	}



	template <typename T>
	const std::string Matrix<T>::outstr() const
	{

		std::string s;
		cout << "cols"<<this->cols<<endl;
		cout << "rows"<<this->rows<<endl;
		for (int i = 0; i < this->rows; i++)
		{
			for (int j = 0; j < this->cols; j++)
			{
				std::ostringstream strs;
				s += " ";
				strs << std::setprecision(8)<<this->v_[i][j];
				s+= strs.str();
			}
			s += " \n";
		}
		return s;
	}


	//inverse
	//TO MODIFY, GENERALIZE
	template <typename T>
	Matrix<T> Matrix<T>::inv()const
	{
		T det;

		Matrix<T> m(this->Rows(), this->Cols());
		Matrix<T> coeff(this->Rows(), this->Cols());

		if (this->rows == 2 && this->cols == 2)
		{
			det = this->v_[0][0] * this->v_[1][1] -
				this->v_[0][1] * this->v_[1][0];


			m.v_[0][0] = this->v_[1][1] / det; m.v_[0][1] = -this->v_[0][1] / det;
			m.v_[1][0] = -this->v_[1][0] / det; m.v_[1][1] = this->v_[0][0] / det;

		}
		else if (this->rows == 3 && this->cols == 3)
        {
            coeff[0][0]= (v_[1][1]*v_[2][2]-v_[1][2]*v_[2][1]);
            coeff[0][1]=-(v_[1][0]*v_[2][2]-v_[1][2]*v_[2][0]);
            coeff[0][2]= (v_[1][0]*v_[2][1]-v_[1][1]*v_[2][0]);

            coeff[1][0]=-(v_[0][1]*v_[2][2]-v_[0][2]*v_[2][1]);
            coeff[1][1]= (v_[0][0]*v_[2][2]-v_[0][2]*v_[2][0]);
            coeff[1][2]=-(v_[0][0]*v_[2][1]-v_[0][1]*v_[2][0]);

            coeff[2][0]= (v_[0][1]*v_[1][2]-v_[0][2]*v_[1][1]);
            coeff[2][1]=-(v_[0][0]*v_[1][2]-v_[0][2]*v_[1][0]);
            coeff[2][2]= (v_[0][0]*v_[1][1]-v_[0][1]*v_[1][0]);

            det=v_[0][0]*coeff[0][0]+v_[0][1]*coeff[0][1]+v_[0][2]*coeff[0][2];

            m=coeff.Tr()*(1.0/det);

            //cout << "Coeff" <<coeff.outstr()<<endl;
        }
        else
        {
            cout << "cannot invert matrix..."<<endl;
        }

		return m;
	}

	// TO MODIFY, GENERALIZE
	template <typename T>
	T Matrix<T>::det()const
	{
		T det;
		Matrix<T> coeff(this->Rows(),this->Cols());

		if (this->rows == 2 && this->cols == 2)
		{
			det = this->v_[0][0] * this->v_[1][1] -
				this->v_[0][1] * this->v_[1][0];

		}
        else if (this->rows == 3 && this->cols == 3)
        {
            coeff[0][0]= (v_[1][1]*v_[2][2]-v_[1][2]*v_[2][1]);
            coeff[0][1]=-(v_[1][0]*v_[2][2]-v_[1][2]*v_[2][0]);
            coeff[0][2]= (v_[1][0]*v_[2][1]-v_[1][1]*v_[2][0]);

            coeff[1][0]=-(v_[0][1]*v_[2][2]-v_[0][2]*v_[2][1]);
            coeff[1][1]= (v_[0][0]*v_[2][2]-v_[0][2]*v_[2][0]);
            coeff[1][2]=-(v_[0][0]*v_[2][1]-v_[0][1]*v_[2][0]);

            coeff[2][0]= (v_[0][1]*v_[1][2]-v_[0][2]*v_[1][1]);
            coeff[2][1]=-(v_[0][0]*v_[1][2]-v_[0][2]*v_[1][0]);
            coeff[2][2]= (v_[0][0]*v_[1][1]-v_[0][1]*v_[1][0]);

            det=v_[0][0]*coeff[0][0]+v_[0][1]*coeff[0][1]+v_[0][2]*coeff[0][2];
        }

		return det;
	}

	template <typename T>
	void Matrix<T>::Set_Determinant()
	{
		T det;
        Matrix<T> coeff(this->Rows(),this->Cols());
		if (this->rows == 2 && this->cols == 2)
		{
			det = this->v_[0][0] * this->v_[1][1] -
				this->v_[0][1] * this->v_[1][0];

		}
        else if (this->rows == 3 && this->cols == 3)
        {
            coeff[0][0]= (v_[1][1]*v_[2][2]-v_[1][2]*v_[2][1]);
            coeff[0][1]=-(v_[1][0]*v_[2][2]-v_[1][2]*v_[2][0]);
            coeff[0][2]= (v_[1][0]*v_[2][1]-v_[1][1]*v_[2][0]);

            coeff[1][0]=-(v_[0][1]*v_[2][2]-v_[0][2]*v_[2][1]);
            coeff[1][1]= (v_[0][0]*v_[2][2]-v_[0][2]*v_[2][0]);
            coeff[1][2]=-(v_[0][0]*v_[2][1]-v_[0][1]*v_[2][0]);

            coeff[2][0]= (v_[0][1]*v_[1][2]-v_[0][2]*v_[1][1]);
            coeff[2][1]=-(v_[0][0]*v_[1][2]-v_[0][2]*v_[1][0]);
            coeff[2][2]= (v_[0][0]*v_[1][1]-v_[0][1]*v_[1][0]);

            this->determ=v_[0][0]*coeff[0][0]+v_[0][1]*coeff[0][1]+v_[0][2]*coeff[0][2];
        }

	}
	//inverse
	//TO MODIFY
	template <typename T>
	Matrix<T> Matrix<T>::Tr()const
	{

		Matrix <T> m(this->cols, this->rows);
		for (int r = 0; r < this->rows;r++)
			for (int c = 0; c < this->cols; c++)
				m.v_[c][r] = this->v_[r][c];

		return m;
	}

    template <typename T>
    void Matrix<T>::Clear()
    {
        T t=0.;
        for (int r = 0; r < this->rows;r++)
			for (int c = 0; c < this->cols; c++)
				this->v_[r][c] = t;
    }

	template <typename T>
	const Matrix<T>
	Matrix<T>::operator+(const Matrix<T> &t2) const
	{
		Matrix <T> m(this->rows, this->cols);
		if (t2.cols==this->cols && t2.rows==this->rows)
		{
			for (int r = 0; r < this->rows;r++)
				for (int c = 0; c < this->cols; c++)
					m[r][c]=this->v_[r][c]+t2[r][c];
		}
		return m;
	}

	template <typename T>
	const Matrix<T>
	Matrix<T>::operator-(const Matrix<T> &t2) const
	{
		Matrix <T> m(this->rows, this->cols);
		if (t2.cols==this->cols && t2.rows==this->rows)
		{
			for (int r = 0; r < this->rows;r++)
				for (int c = 0; c < this->cols; c++)
					m[r][c]=this->v_[r][c]-t2[r][c];
		}
		return m;
	}

	template <typename T>
	const Matrix<T>
	Matrix<T>::operator*(const T &d) const
	{
		Matrix <T> m(this->rows, this->cols);
		for (int r = 0; r < this->rows;r++)
			for (int c = 0; c < this->cols; c++)
				m[r][c]=this->v_[r][c]*d;

		return m;
	}

	template <typename T>
	void
	Matrix<T>::SetIdentity()
	{
		this->Clear();
		if (this->rows==this->cols)
		{
			T t=1.;
			for (int i=0;i<this->rows;i++)
				this->v_[i][i]=t;

		}
	}

}//FluxSol
#endif
