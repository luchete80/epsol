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

///////////////
// EXAMPLE 1 //
///////////////
// Viwcoplastic Problem
// 1 Elements,4 node each, loaded as follows
//FORCEX
// ---------------
// |             |             
// |             |             
// |      1      |           
// |             |             
// |             |             
// ---------------
// FIX_XY		FIX_Y

//Solved with Laspack Library

#include "EPSol.h"

//Sparse libraries
//#include "laspack.h"

#include <iostream>
#include <vector>

using namespace std;
using namespace FluxSol;

class Eulerian_ViscoPlastic{

public:
    Eulerian_ViscoPlastic();
	void calc_matrices();
    void setup();
	void assemble();
	void calc_res();

private:

	FeGrid <2> grid;
	FluxSol::FEValues<2> fev;
//Matrices
//Gauss

	std::vector<int> var_dim(4);
	std::vector<int> vnrow(20),vncol(20);
// ELEMENTAL MATRICES AND VECTORS
    GaussFullMatrices B,J,dHdrs;
    //Shape
    Matrix<double> Nv,NsigF,Ns;//Nv,Nsig,NF,Ns;
    
	//Shape derivatives
	Matrix<double> Bv,Bs,LM;
	vector < Matrix <double> >BsigF, BL;
	Matrix <double> temp4x16;

	FluxSol::Matrix<double> G,H,Kel;
	
	//Vectors
	//Solution
	Matrix<double> U,UV,Usig,UF,Us,Ft;
	//Deformation gradients
	Matrix<double> F,P,L;	//Unsymmetric vectors
	
	//Stress
	Matrix<double> sig,sig_d,N_d;	//Unsymmetric vectors
	
	//Residual 
	vector < vector <Matrix <double> > > Kt; 

	vector < Matrix<double> > R;			//Nv,Nsig,NF,Ns;
	
	Vector <double> dVxy;
	
	Matrix <double> Kglob;
	Vector <double> Uglob;
		
	//Dimensions
	int dof[4];
	//enum field;
	
	//Material
	Matrix<double>c;
	double Ey,nu;

	//Global matrices
	//Uglob=matrix(numpy.matlib.zeros((ndof*numnodes, ndof*numnodes)))
	//Uglob=zeros(dof)
	int form;	//Formulation

    ofstream logfile;
};


void Eulerian_ViscoPlastic::setup()
{
	var_dim={2,4,4,1}; //Fails
	ofstream logfile;
	logfile.open("Logfile.txt");
	
	//Matrices
	//Constants should be defined here
	G=H=Matrix<double>(4,4);
	G[0][0]=G[1][3]=G[2][3]=G[2][1]=1.;
	H[0][0]=G[0][1]=1.;
	
	Ey = 200.0e9;
	nu = 0.33;
	
	//Material
    c=Matrix<double>(4,4);
	double ck=Ey/((1.+nu)*(1.-2.*nu));
	c[0][0]=c[1][1]=c[2][2]=ck*(1.-nu);
	c[0][1]=c[0][2]=c[1][0]=c[2][0]=c[1][2]=c[2][1]=ck*(1.-nu);
	c[2][2]=ck*(0.5-nu);
	
	const FluxSol::FEIntegrationScheme intsch(1,2);


//	e.Set_Nodes(4, 0, 1, 2, 3);
	//logfile << "Element Gauss Order: " << e.GaussOrder() << "\n\n";
	
	//Shape functions 
	//Nv = e.H();
	Bv=Matrix<double>(4,8);

	std::vector< FluxSol::Element<2> > ve;

	this->grid=FeGrid <2> (1.,1.,1.,10,10,1);
   
    dHdrs=fev.shape_localgrad_matrix(); //COMPLETE
    ShapeFunctionGroup shfngr = this->grid.Elem(0).CreateShapeFunctionGroup();
	// Shape Functions
	//To MODIFY REFER TO dim
	//Shape and gradient MATRICES
	//These are not grouped into an array because
	//1: Two of them are equal
	//2: Bsig and BF are THREE DIM ARRAY
    Nv	 =Matrix<double>(2, 8);//v
	NsigF=Matrix<double>(4,16);//sig
	Ns	 =Matrix<double>(1, 4);//s

	//Solution (Fig 2.1)
	dof[0]=8;		//Velocity: dim=2 x 4 nodes
	dof[1]=dof[2]=16;	//Deformation
	dof[3]=4;
	//R   =Matrix<double>(11,1);
	for (int i=0;i<4;i++)
		R.push_back(Matrix<double>(dof[i],1));
	//Deformation gradients
	//Vector<double> F,P,L;	//Unsymmetric vectors
	
	// #Nodal values
	// Ve=Matrix<double>(8, 1)))


	// #Derivatives
	// dHxy=Matrix<double>(2, 4)))
	// Bs=Matrix<double>(2, 8)))
	// Bv=Matrix<double>(4, 8)))
	// #BsigF=[Matrix<double>(4, 16))),Matrix<double>(4, 8)))]
	// BsigF=arange(128).reshape(4,16,2) #
	// temp4x16=Matrix<double>(4, 16)))
	// B4i=arange(32).reshape(4,4,2) #
	vector < vector <vector < double > > > B4i;
	// #(4,16,2)
	// print(BsigF[0])
	for (int i=0;i<2;i++)	BsigF.push_back(Matrix<double> (4, 16));
	temp4x16=Matrix<double>(4, 16);
	
	LM=Matrix<double>(4, 4);

	// //R =Matrix<double>(44, 1)
	// RF  =Matrix<double>(16, 1);
	// Rsig=Matrix<double>(16, 1);
	// Rs  =Matrix<double>(4, 1);
	// Rv  =Matrix<double>(8, 1);

	// U =Matrix<double>(44, 1)))
	UF  =Matrix<double>(16, 1);
	Usig=Matrix<double>(16, 1);
	//dVxy=Matrix<double>(4, 2);

	// #Symmetric tensors
	E=Matrix<double>(4, 1);

	// #Stress
	sig=Matrix<double>(4, 1);   //Stress Gauss Points
	sig_d=Matrix<double>(4, 1); //Deviatoric

	P=Matrix<double>(4, 1);


	v=Matrix<double>(2, 1);    //Gauss Point velocity			


	Kel=Matrix<double> (44, 44);
	
	// for (int i=0;i<4;i++)
		// for (int j=0;j<4;j++)
			// for (int k=0;k<4;k++)
				// B4i.push_back(0.)			
	for (int i=0;i<4;i++)
		BL.push_back(Matrix<double>(4,2));

	//These are the same but reorganized
	dVxy=Vector<double>(4);
	L   =Matrix<double> (2, 2);
	//BL      = arange(128).reshape(4,4,8)            #Eqns 2.33, B.17
	temp8x1 = Vector<double> (8,1);


	//Stress
	sig  =Matrix<double>(4, 1);     //#Stress Gauss Points
	sig_d=Matrix<double>(4, 1);     //#Deviatoric, is also symmetric
	N_d  =Matrix<double>(4, 1);  	//#Direction of plastic strain

	P=Matrix<double>(4, 1); 
	
}

void Eulerian_ViscoPlastic::calc_matrices()
{

	
}

void Eulerian_ViscoPlastic::assemble()
{
	
	for (int e=0;e<this->grid.NumElem();e++){


		for (int n=0;n<4;n++){
			int d=elnodes[e][n];
			for (int i=0;i<var_dim[0];i++)
				UV[i][0]=Uglob[ndof*d+i];
			for (int j=0;j<var_dim[1];j++)
				Usig[i,0]=Uglob[ndof*d+2+j];
				if (form==1)
					Usig[j][0]=Uglob[ndof*d+var_dim[0]+j];
					UF  [j][0]=Uglob[ndof*d+6+j];
				else
					UF  [j][0]=Uglob[ndof*d+2+j];
			for (int j=0;j<var_dim[2];j++)
				UFvp[j,0]=Uglob[ndof*d+var_dim[0]+var_dim[1]+j];			
				}
				
		v=Nv*UV;
		s=Ns*Us;
		F=NsigF*UF;

		if (form==1):
			sig=NsigF*Usig;
		else:
			Fvp=NFvp*UFvp;	
		
		//C = E / (1 - nu*nu)*[1 nu 0; nu 1 0; 0 0 (1 - nu) / 2];
		cout << "Num Integration Points"<<intsch.NumPoints()<<endl;

		this->fev =FEValues<2>(this->grid.Elem(e), this->grid);
		
		J = fev.Jacobian();
		B = fev.shape_grad_matrix();
	
		for (int g = 0; g < intsch.NumPoints(); g++)
		{
			Bs=this->fev.shape_grad_comps(g);
			for (int i=0;i<4;i++){ 
			Bv[0][2*i  ]=B[2][2*i]=Bs[0][i];
			Bv[1][2*i+1]=B[2][2*i]=Bs[0][i];}


			for (int k=0;k<4;k++){
			//shape functions
			Nv[0][2*k ]=Nv[1][2*k+1]=Ns[0,k];
			//derivatives Bv (B.14)
			Bv[0][2*k  ]=Bs[0,k];
			Bv[1][2*k  ]=Bs[1,k];
			Bv[2][2*k+1]=Bs[0,k];
			Bv[3][2*k+1]=Bs[1,k];}
			
			for (int k=0;k<8;k++){
				BL[1][1][k]=BL[3][3][k]=Bv[1][k];
				BL[1][2][k]=BL[3][4][k]=Bv[3][k];
				BL[2][1][k]=BL[4][3][k]=Bv[2][k];
				BL[2][2][k]=BL[4][4][k]=Bv[4][k];
				BL[1][3][k]=BL[1][4][k]=BL[2][3][k]=BL[2][4][k]=BL[3][1][k]=BL[3][2][k]=BL[4][1][k]=BL[4][2][k]=0.;}
			
			for (int i=0;i<4;i++){
				for (int l=0;l<4;l++){
					for (int m=0;m<4;m++)
						for (int n=0;n<4;n++)
							if (l==m)
								B4i[l,m,n]=Bs[n,i];
							else
								B4i[l,m,n]=0.;  
							
			for (int l=0;l<4;l++)
				for (int m=0;m<4;m++) 
					for (int n=0;n<4;n++)
						BsigF[4*i+l][m][n]=B4i[l][m][n];}}//For i 
			
				
			//FluxSol::Matrix<double> Kg = B.Mat(g).Tr()*c*B.Mat(g);
			for (int r = 0; r < 8; r++)
				for (int c = 0; c < 8; c++){
					Kel[r][c] += Kg[r][c] * intsch[g].w()*J.Mat(g).det();
					cout << "weight" << intsch[g].w()<<endl;
					cout << "Kel: "<< Kel[r][c]<<endl;	}
					
					
			int ir,ic,imax,jmax;
			int vrowinc,vcolinc;
			int vrowinc=0;
			int d;
			//#Assembly Matrix
			for (int vrow=0;vrow<4;vrow++){
				ir=0;
				imax=var_dim[vrow];
				for (int n=0;n<4;n++)//Nodes
						for (int i=0;i<imax;i++){
						d=elnodes[e][n];
						//print("vrowinc,d,a+b",vrowinc,d,vrowinc+var_dim[vrow]*d+i)
						vnrow[ir]=vrowinc+var_dim[vrow]*d+i;
						ir=ir+1;}
        
				vcolinc=0;        
				for (int vcol=0;vcol<4;vcol++){
					//print("vcol",vcol)
					jmax=var_dim[vcol];
					//print("imax, jmax",imax,jmax)
					//Store vn vectors
					ic=0;
					for (int n=0;n<4;n++)//Nodes
						for (int j=0;j<jmax;j++){
							d=elnodes[e][n]
							//print("vcolinc",vcolinc)
							vncol[ic]=vcolinc+var_dim[vcol]*d+j;
							ic=ic+1;}
							
						
					//print("vnrow",vnrow.astype(int))            
					//print("vncol",vncol.astype(int))
					for (int row=0;row<(4*imax);row++) 
						for (int col=0;col<(4*jmax);col++)
							Kglob[vnrow[row],vncol[col]]=  
									Kglob[vnrow[row]][vncol[col]]+
									Kt[vrow][vcol][row][col];
									
					vcolinc+=numnodes*var_dim[vcol];}//vcol
					
				vrowinc+=numnodes*var_dim[vrow];}//vrow
		}//gauss points
	}

}//Element
	
	logfile << "Stiffness Matrix \n\n";
	logfile << Kel.outstr();

	//DOFs 5, 6 and 8 restricted
	for (int i = 0; i < 8; i++)
	{
		Kel[4][i] = Kel[i][4] = 0.0;
		Kel[5][i] = Kel[i][5] = 0.0;
		Kel[7][i] = Kel[i][7] = 0.0;
	}
	Kel[4][4] = Kel[5][5] = Kel[7][7] = 1.;
	// //////////////////// LASPACK SOLVER ///////////////////////

	PETSC_Solver<double,2> solver(8);

	
	//DOFs 5, 6 and 8 restricted
	for (int i = 0; i < 8; i++)
	{
		Kel[4][i] = Kel[i][4] = 0.0;
		Kel[5][i] = Kel[i][5] = 0.0;
		Kel[7][i] = Kel[i][7] = 0.0;
	}
	Kel[4][4] = Kel[5][5] = Kel[7][7] = 1.;
	
	for (int row = 0; row < 8; row++){
		for (int col = 0; col < 8; col++)
		{
			solver.AddMatVal(row,col,Kel[row][col]);
		}

	}
	cout<<Kel.outstr();
	solver.Flush();
	
	solver.SetbValues(2,1000.0);
	

	solver.ViewInfo();
	solver.Solve();
	solver.ViewInfo();
	
	logfile.close();
	return 0;


}

void Eulerian_ViscoPlastic::calc_res()
{
	//R[0]=B[0].tr()*P - N[0].tr()*tP;
	// Rsig j = Bsig mjk * vk * Usig j - C mj E.(e)j   
	//R[1]=(N[1].tr()+tau(e)*v) * ()
	
}
