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
// EXAMPLE 2 //
///////////////
#include "EPSol.h"

// Plain Stress / Strain Problem
// 10 x 10 Elements,4 node each, loaded as previous example
using namespace FluxSol;

class outfile
{
	protected:
	//To check if new fields have same mesh
	//Fv_CC_Grid &grid;
	vector <Vec3D> node_data;   //Results
	//vector <> cell_data;		//Element results

	public:
	//Constructors
	outfile(string name, FeGrid &grid<2>);

}
	
int main()
{

	ofstream logfile;
	logfile.open("Logfile.txt");
	

	FEIntegrationScheme intsch(1,2);
	

	//Mesh
	//FeGrid(const double &lex, const double &ley, const double &lez,
	FluxSol::FeGrid<2> grid(1.,1.,1.,2,1,1);
	
	// FluxSol::FeGrid<2> grid;
	// grid.Create_test(1.,1.,1.,2,2,1);
	cout << "Total Grid Nodes" <<grid.NumNodes()<<endl;
			for (int n = 0; n < grid.NumNodes(); n++)
				cout<<grid.Nod(n).Coords()<<endl;
			
	grid.outstr();

	FluxSol::DoFHandler<2> dofhandler(grid);
	cout << "Global DOFs" << dofhandler.NumDoF()<<endl;
	
	dofhandler.outstr();

	cout << "Assemblying matrix" <<endl;
	
	FluxSol::GaussFullMatrices J,B,dHdrs;
	//FluxSol::ShapeFunctionGroup shfngr = e.CreateShapeFunctionGroup();

	FluxSol::Matrix<double> Kel(8, 8);
	FluxSol::Matrix<double> c(3, 3);

	PETSC_Solver<double,2> solver(dofhandler.NumDoF());
    Matrix<double> Kgi(dofhandler.NumDoF(),dofhandler.NumDoF());	

	double E = 200.0e9;
	double nu = 0.33;

	//Plain Stress
	double ck = E / (1 - nu*nu);

	ck = E*(1.-nu)/((1.+nu)*(1.-2*nu));
	c[0][0] = c[1][1] = ck;
	c[0][1] = c[1][0] = ck*nu/(1.-nu);
	c[2][2] = ck*(1 - 2*nu) / (2.*(1.-nu));
	

	for (int e=0;e<grid.NumElem();e++)
	{
		cout <<"Element: " << e <<endl;
		// TO BE MODIFIED: DONT INCLUDE GRID
        vector <unsigned int> vn = dofhandler.get_dof_indices(e);

		for (int i=0;i<vn.size();i++)
			cout<<"GLOBAL DOFS"<<vn[i]<<endl;
		
		cout << "Creating values ..."<<endl;
		//cout << "Element nodes:"<<grid.Elem(e).NumNodes()<<endl;
		//cout << grid.XYZ(grid.Elem(e)).outstr()<<endl;
		FluxSol::FEValues<2> fev(grid.Elem(e),grid);
		//FluxSol::FEValues<2> fev(el,grid);
		cout << "Element " << e << "Gauss Order: "<<grid.Elem(e).GaussOrder()<<endl;
		
        J = fev.Jacobian();
		B = fev.shape_grad_matrix();
		
		B.outstr();
        Kel=0.;
		cout << "Creating Elemental Stiffness Matrix, GaussPoints: "<< intsch.NumPoints() <<endl;
        for (int g = 0; g < intsch.NumPoints(); g++)
        {
			cout << "J matrix " << J.Mat(g).outstr()<<endl;
			cout << "B matrix " << B.Mat(g).outstr()<<endl;
			
            FluxSol::Matrix<double> Kt = B.Mat(g).Tr()*c*B.Mat(g);
            for (int r = 0; r < 8; r++)
				for (int c = 0; c < 8; c++)
					Kel[r][c] += Kt[r][c]*intsch[g].w()*J.Mat(g).det();
			
			cout <<Kt.outstr();
        }
		
		cout <<Kel.outstr();
		
		logfile << "Stiffness Matrix \n\n";
		logfile << Kel.outstr();

		cout <<"Dof Indices"<<endl;
		for(int row = 0; row < 8; row++)
			cout <<vn[row]<<endl;
				
        for (int row = 0; row < 8; row++){
            for (int col = 0; col < 8; col++){			
				solver.AddMatVal(vn[row],vn[col],Kel[row][col]);
            }

        }

	}
	
	solver.Flush();
	
	cout << "Applying BCs..."<<endl;
	
	int dof;
	int fix[]={0,1,5};
	for (int f=0;f<3;f++){
		dof=fix[f];
		cout << "adj dofs"<<endl;
		for (int i = 0; i < dofhandler.Adj_DoF_Number()[dof]; i++){
			int adjdof = dofhandler.AdjDoF(dof,i);
			cout << adjdof << " ";
			//Have to set up only the nonzero matrix values
			solver.SetMatVal(dof,adjdof,0.);
			solver.SetMatVal(adjdof,dof,0.);}
		solver.SetMatVal(dof,dof,1.);	}	

				
	solver.SetbValues(6,1000.0);
	

	solver.ViewInfo();
	solver.Solve();
	solver.ViewInfo();
	
	logfile.close();
	
	outfile of("output.vtk",grid);
	
	logfile << "Stiffness Matrix \n\n";
	logfile << Kel.outstr();

	
	logfile.close();
	return 0;
}


outfile::outfile(string name, FeGrid &grid<2>)
{
	int Rank=0;
	
	// string fileName=name;
	// ofstream file;
	// file.open((fileName).c_str(),ios::out);
	// file << "<?xml version=\"1.0\"?>" << endl;
	// file << "<VTKFile type=\"UnstructuredGrid\">" << endl;
	// file << "<UnstructuredGrid>" << endl;
	// file << "<Piece NumberOfPoints=\"" << grid.Num_Verts() << "\" NumberOfCells=\"" << grid.Num_Cells() << "\">" << endl;
	// file << "<Points>" << endl;
	// file << "<DataArray NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" >" << endl;
	// for (int n=0;n<grid.Num_Verts();++n) {
		// for (int i=0; i<3; ++i) file<< setw(16) << setprecision(8) << scientific << grid.Node(n).Comp()[i] << endl;
	// }
	// file << "</DataArray>" << endl;
	// file << "</Points>" << endl;
	// file << "<Cells>" << endl;
	
	// file << "<DataArray Name=\"connectivity\" type=\"Int32\" format=\"ascii\" >" << endl;
	// for (int c=0;c<grid.Num_Cells();++c) {
		// for (int n=0;n<grid.Cell(c).Num_Vertex();++n) {
			// file << grid.Cell(c).Vert(n) << "\t";}
		// file << endl;}
	
	// file << "</DataArray>" << endl;
	// file << "<DataArray Name=\"offsets\" type=\"Int32\" format=\"ascii\" >" << endl;
	// int offset=0;
	// for (int c=0;c<grid.Num_Cells();++c) {
		// offset+=grid.Cell(c).Num_Vertex();
		// file << offset << endl;}
	// file << "</DataArray>" << endl;
	
	// file << "<DataArray Name=\"types\" type=\"UInt8\" format=\"ascii\" >" << endl;
	// for (int c=0;c<grid.Num_Cells();++c) {
		// if (grid.Cell(c).Num_Vertex()==4) file << "10" << endl; // Tetra
		// if (grid.Cell(c).Num_Vertex()==8) file << "12" << endl; // Hexa
		// if (grid.Cell(c).Num_Vertex()==6) file << "13" << endl; // Prism
		// if (grid.Cell(c).Num_Vertex()==5) file << "14" << endl; // Pyramid (Wedge)
	// }
	// file << endl;
	// file << "</DataArray>" << endl;;
	
	// file << "</Cells>" << endl;
	
	// file << "<CellData Scalars=\"scalars\" format=\"ascii\">" << endl;
	
	// //Begin data field output
	// file << "<DataArray Name=\"";

	// file << "Var";
	// file << "\" type=\"Float32\" format=\"ascii\" >" << endl;

	// //If scalars
	// for (int n=0;n<grid.Num_Nodes();n++)
		// file << field.Val(n).Comp()[0] << endl;

	// file << "</DataArray>" << endl;

	// // End of data output
	// file << "</CellData>" << endl;
	
	// file << "</Piece>" << endl;
	// file << "</UnstructuredGrid>" << endl;
	// file << "</VTKFile>" << endl;
	// file.close();

}