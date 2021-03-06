# https://docs.sympy.org/latest/tutorial/matrices.html
#Plain strain/stress 1 element example
#import math
from numpy import *
import numpy.matlib #for zeros

import array as arr

import deriv

#----------------------------
#Input Data------------------
form=2
lx=1.
ly=1.
nex=2
ney=2
#-------------


#***** MESH**********
dx=lx/nex
dy=ly/ney
numel=nex*ney
vn=zeros(8) #Dof connectivity
numnodes=(nex+1)*(ney+1)
node=zeros((numnodes, 2))
elnodes=zeros((numel, 4))#Connectivity
elnodes.astype(int)
#print (node)
#Mesh generation 
y=0.
n=0
for nx in range (ney+1):
    x=0.
    for ny in range (nex+1):
        #print("Node: "+str(x)+" , "+str(y))
        #this->node.push_back(Node(n,x,y,0.0))
        node[n]=[x,y]
        x=x+dx
        n=n+1
    y=y+dy
    
print(node)
#Connectivity
e=0
for ey in range (ney):
    for ex in range (nex):
        elnodes[e]=[(nex+1)*(ey+1)+ex+1,
                    (nex+1)*(ey+1)+ex,
                    (nex+1)*ey + ex,
                    (nex+1)*ey + ex+1]
        e=e+1
print(elnodes)
#-------------------------- MESH
#Formulation type and DOFs
if form==1:
    ndof =11
else:
    ndof =12

#Element dimension and DOF PER VARIABLE! 
var_dim =[2,4,4,1]
var_edof=zeros(4)
if form==2:
    var_dim =[2,4,5,1]
for i in range(4):
    var_edof[i]=4*var_dim[i]  
    
print ("var_edof",var_edof)

edof=ndof*4
dof=ndof*numnodes
#X is combines per row to calculate J
p=1.0/1.732050807568877
gauss=[-p,p]
#Numerated as in Bathe
X2=numpy.matlib.zeros((4, 2))

#Main variables values
#Nodal values
U   =matrix(numpy.matlib.zeros((44, 1)))
UV  =matrix(numpy.matlib.zeros((8, 1)))
Usig=matrix(numpy.matlib.zeros((16, 1)))
UF  =matrix(numpy.matlib.zeros((16, 1)))
UFvp=matrix(numpy.matlib.zeros((20, 1)))
Us  =matrix(numpy.matlib.zeros((4, 1)))

#Gauss variables
#d vectors are [xx yx xy yy zz]
F       = matrix(numpy.matlib.zeros((4,1)))#Tensor form
Ft      = matrix(numpy.matlib.zeros((3,3)))#Tensor form
Fd      = matrix(numpy.matlib.zeros((4,1)))#Vector form, FZZ IS 1!!
Ft_inv  = matrix(numpy.matlib.zeros((3,3)))#Tensor form
Fd_inv  = matrix(numpy.matlib.zeros((5,1)))#Tensor form

#Formulation 2
Fvp  = matrix(numpy.matlib.zeros((4,1)))#Tensor form
Fvpt = matrix(numpy.matlib.zeros((3,3)))#Tensor form
Fvpd = matrix(numpy.matlib.zeros((5,1)))#Tensor form

S=matrix(numpy.matlib.zeros((4, 1)))    #Nodal internal variable
v=matrix(numpy.matlib.zeros((2, 1)))    #Gauss Point velocity

#Shape Functions
Ns=matrix(numpy.matlib.zeros((1, 4)))
Nv=matrix(numpy.matlib.zeros((2, 8)))
NsigF=matrix(numpy.matlib.zeros((4, 16)))
NFvp=matrix(numpy.matlib.zeros((5, 20)))

Dvp=matrix(numpy.matlib.zeros((4, 1)))      #From plastic deformation direction
DM =matrix(numpy.matlib.zeros((5, 5)))   #4.24
temp2x2=matrix(numpy.matlib.zeros((2, 2)))

#Derivatives
dHxy=matrix(numpy.matlib.zeros((2, 4)))
Bs=matrix(numpy.matlib.zeros((2, 8)))
Bv=matrix(numpy.matlib.zeros((4, 8)))
#BsigF=[matrix(numpy.matlib.zeros((4, 16))),matrix(numpy.matlib.zeros((4, 8)))]
BsigF=arange(128).reshape(4,16,2) #
BFvp =arange(200).reshape(5,20,2) #

temp4x16=matrix(numpy.matlib.zeros((4, 16)))
temp4x1=matrix(numpy.matlib.zeros((4, 1)))
temp4x8=matrix(numpy.matlib.zeros((4, 8))) #BL*NFUF
temp5x16=matrix(numpy.matlib.zeros((5, 20)))

temp4x2=matrix(numpy.matlib.zeros((4, 2)))
temp20x2=matrix(numpy.matlib.zeros((20, 2))) #For 4.39

B4i=arange(32).reshape(4,4,2) #
B5i=arange(50).reshape(5,5,2) #
#(4,16,2)
#print(BsigF[0])
LM=matrix(numpy.matlib.zeros((4, 4)))
#Only returns derivatives respectives to F and Fvp
dEdU=[matrix(numpy.matlib.zeros((4, 16))),matrix(numpy.matlib.zeros((4, 20)))]

K=matrix(numpy.matlib.zeros((44, 44)))
K=matrix(numpy.matlib.zeros((44, 44)))

R   =matrix(numpy.matlib.zeros((44, 1)))
RF  =matrix(numpy.matlib.zeros((16, 1)))
Rsig=matrix(numpy.matlib.zeros((16, 1)))
RFvp=matrix(numpy.matlib.zeros((20, 1)))
Rs  =matrix(numpy.matlib.zeros((4, 1)))
Rv  =matrix(numpy.matlib.zeros((8, 1)))

#Formulation 1
#-----------------------------------
#Symmetric tensors
Ee =matrix(numpy.matlib.zeros((4, 1)))
Eet=matrix(numpy.matlib.zeros((2, 2))) #Tensor form 



#These are the same but reorganized
dVxy=zeros(4)
L   =matrix(numpy.matlib.zeros((2, 2)))
BL      = arange(128).reshape(4,4,8)            #Eqns 2.33, B.17
temp8x1 = matrix(numpy.matlib.zeros((8, 1))) 


#Stress
sig  =matrix(numpy.matlib.zeros((4, 1)))    #Stress Gauss Points
sig_d=matrix(numpy.matlib.zeros((4, 1)))    #Deviatoric, is also symmetric
N_d  =matrix(numpy.matlib.zeros((4, 1)))    #Direction of plastic strain

P=matrix(numpy.matlib.zeros((4, 1))) 

#Global matrices
#Uglob=matrix(numpy.matlib.zeros((ndof*numnodes, ndof*numnodes)))
Uglob=zeros(dof)

class bMatrix: #Block matrix
    
    def __init__(self,i,j):
        m=matrix(numpy.matlib.zeros((i, j)))

    # def show_all(self):
        # print(self.left, self.right)
#Matrix Tangent 
#U derivatives
#
# Kt_U=[[matrix(numpy.matlib.zeros(( var_edof.astype(int)[0], var_edof.astype(int)[0]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[0], var_edof.astype(int)[1])))],
      # [matrix(numpy.matlib.zeros(( var_edof.astype(int)[0], var_edof.astype(int)[2]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[0], var_edof.astype(int)[3])))]]
   
#Matrix Double Entry
#Example in formulation 1:
# ( 8x 8)  (8x16)  (8x20) (8x4)
# (16x 8) (16 x 16) ..  ..
# (20x 8) (20x16) (20x20) (20x4)
# ( 4x 8)

dDdUv   =arange(200).reshape(5,5, 8) #TO MODIFY
dDdUF   =arange(400).reshape(5,5,16) #TO MODIFY
dDdUFvp =arange(500).reshape(5,5,20) #TO MODIFY
dDdUs   =arange(100).reshape(5,5, 4) #TO MODIFY

#From multiplying dDdU x NFvp*
#4.39 to 4.42
temp_dDFvp=[numpy.matlib.zeros((5, 8)),
            numpy.matlib.zeros((5,16)),
            numpy.matlib.zeros((5,20)),
            numpy.matlib.zeros((5, 4))]

Kt=[
     [matrix(numpy.matlib.zeros(( var_edof.astype(int)[0], var_edof.astype(int)[0]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[0], var_edof.astype(int)[1]))),
      matrix(numpy.matlib.zeros(( var_edof.astype(int)[0], var_edof.astype(int)[2]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[0], var_edof.astype(int)[3])))]
     ,
     [matrix(numpy.matlib.zeros(( var_edof.astype(int)[1], var_edof.astype(int)[0]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[1], var_edof.astype(int)[1]))),
      matrix(numpy.matlib.zeros(( var_edof.astype(int)[1], var_edof.astype(int)[2]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[1], var_edof.astype(int)[3])))
     ],
     [matrix(numpy.matlib.zeros(( var_edof.astype(int)[2], var_edof.astype(int)[0]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[2], var_edof.astype(int)[1]))),
      matrix(numpy.matlib.zeros(( var_edof.astype(int)[2], var_edof.astype(int)[2]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[2], var_edof.astype(int)[3])))]
     ,
     [matrix(numpy.matlib.zeros(( var_edof.astype(int)[3], var_edof.astype(int)[0]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[3], var_edof.astype(int)[1]))),
      matrix(numpy.matlib.zeros(( var_edof.astype(int)[3], var_edof.astype(int)[2]))), matrix(numpy.matlib.zeros((var_edof.astype(int)[3], var_edof.astype(int)[3])))
     ]    
    ]     
dgdU=[  matrix(numpy.matlib.zeros((1, 8))),
        matrix(numpy.matlib.zeros((1,16))),
        matrix(numpy.matlib.zeros((1,20))),
        matrix(numpy.matlib.zeros((1, 4)))]

print("Kt_0",len((Kt[1][0])),len((Kt[1][0]).transpose()))

#Material properties (Table 2.1 p28)
#HSLA-65 steel with strain rate between e-3 and e-4
mat_A0 =6.34e11
mat_psi=3.25
mat_m  =0.1956
mat_n  =0.06869
mat_s0 =80.0e6
mat_Q  =312.35e3
mat_R  =8.314
mat_a  =1.5
mat_h0 =3093.1e6
mat_s0 =125.1e6

# print (K)
# print (X2[0,0])
# print (X2[2,1])

ey=200.e9
nu=0.33

ck = ey/ ((1. + nu)*(1. - 2. * nu))
c=matrix(numpy.matlib.zeros((4, 4)))
c[0,0]=c[1,1]=c[2,2]=ck*(1.-nu)					
c[0,1]=c[1,0]=c[0,2]=c[2,0]=c[2,1]=c[1,2]=ck*nu
c[3,3]=ck*(.5 - nu)
print ("C matrix")
print(c)
#Set G and H matrices
#2.32
G=matrix([[1,0,0,0],[0,0,0,1],[0,0,0,1],[0,1,0,0]]) 
H=matrix([[1,0,0,0],[0,1,0,0],[0,0,0,0],[0,0,0,0]]) 

#ELEMENT LOOP  
it=0
for e in range (4):
    #Obtain Ve from global
    Kel=0.
    for n in range(4):
        X2[n]=node[elnodes.astype(int)[e][n]]
    print ("Element ", e)
    print ("Element Nodes")
    print (X2)
    
    print ("Nv",Nv)
    for ig in range(2):
        for jg in range(2):
            rg=gauss[ig]
            sg=gauss[jg]

            #Numerated as in Bathe
            Ns  =0.25*matrix([(1+sg)*(1+rg),(1-rg)*(1+sg),(1-sg)*(1-rg),(1-sg)*(1+rg)])   
            dHrs=matrix([[(1+sg),-(1+sg),-(1-sg),(1-sg)], [(1+rg),(1-rg),-(1-rg),-(1+rg)] ])
            #Numerated as in deal.ii
            #dHrs=matrix([[-(1-s),(1-s),-(1+s),(1+s)], [-(1-r),-(1+r),(1-r),(1+r)] ])        
            dHrs/=4.
            J=dHrs*X2
            dHxy=linalg.inv(J)*dHrs
            detJ=linalg.det(J)
            #Calculate shape functions
            #Bs=J-1 dHrs(B.13)
            Bs=dHxy
            for k in range(4):
                #shape functions
                Nv[0,2*k  ]=Nv[1,2*k+1]=Ns[0,k]
                for j in range(4):
                    NsigF[j,4*k+j]=Ns[0,k]

                #derivatives Bv (B.14)
                Bv[0,2*k  ]=dHxy[0,k]
                Bv[1,2*k  ]=dHxy[1,k]
                Bv[2,2*k+1]=dHxy[0,k]
                Bv[3,2*k+1]=dHxy[1,k]
            
            ################            
            ## 
            for i in range(4):
                for l in range(4):
                    for m in range(4):  
                        for n in range(2):
                            if (l==m):
                                B4i[l,m,n]=Bs[n,i]
                            else:
                                B4i[l,m,n]=0.              
                for l in range(4):
                    for m in range(4):  
                        for n in range(2):
                            BsigF[l,4*i+m,n]=B4i[l,m,n]
            
            ###################
            ## Ec. D.20 p177 
            if form==2:
                for i in range(4):
                    for l in range(5):
                        for m in range(5):  
                            for n in range(2):
                                if (l==m):
                                    B5i[l,m,n]=Bs[n,i]
                                else:
                                    B5i[l,m,n]=0.              
                    for l in range(5):
                        for m in range(5):  
                            for n in range(2):
                                BFvp[l,4*i+m,n]=B5i[l,m,n]            
                            
            #Interpolate velocity
            #INCREMENT GLOBAL VELOCITY FROM INCREMENTS!!!
            #CHANGE F TO ASSEMBLE IN THE SAME PLACE FOR BOTH FORMS
            for n in range (4):
                d=elnodes.astype(int)[e][n]
                #print("d len Uglob i",d,len(Uglob),ndof*d+2)
                for i in range (var_dim[0]):    #Velocity is var 0
                    UV[i,0]=Uglob[ndof*d+i]
                for j in range (var_dim[1]):
                    #print("J",j)
                    if (form==1):
                        Usig[j,0]=Uglob[ndof*d+var_dim[0]+j]
                        UF  [j,0]=Uglob[ndof*d+6+j]
                    else: #Fig 4.1, Z is not translated to Fvpt
                        UF  [j,0]=Uglob[ndof*d+var_dim[0]+j]
                for j in range (var_dim[2]):
                        UFvp[j,0]=Uglob[ndof*d+var_dim[0]+var_dim[1]+j]
                        
            v  =Nv*UV #[2x8 x (8x1)]
            s  =float(Ns*Us)
            F  =NsigF*UF #[(4x16)*(16x1) =(4x1)]
            #Ft=
            if (form==1):
                sig=NsigF*Usig
            else:
                Fvp=NFvp*UFvp
            
             
            #Galerkin strain integration
            #Calculate deformation gradient Fij, for that
            #Calculate Velocity gradient Lij
            #Lij=dvi/dxj(2.4) 
            #According to B.11 d(phi)/dxj=J-1(ij) dphi/drj = Bvjk Vk
            
            #Formulation 1
            #Calculate La (ij) 2.9 
            #Laij=F(-1)ki F(-1)kl Le lj
            
            #Calculate Leij = Lij - D(th)ij - D(vp) ij (2.10-2.12)
            #Calculate Almansi deformation gradient E (A.5)
            #Calculate 
            #Ea ij= 1/2(Lki F(-1)lk F(-1)LJ +F(-1)ki F(-1)KL Llj )
            dVxy=Bv*UV #(4x8)*(8x1)=(4x1) (vx,x vx,y vy,x vy,y)T 
            L[0,0]=dVxy[0]
            L[0,1]=dVxy[1]
            L[1,0]=dVxy[2]
            L[1,0]=dVxy[3]
            
            #Stabilization factor tau 2.26
            #tau=beta*he/(2|v|)
            #See beta estability paramter
            #LM (2.33 p23)
            #Attention: SE DIFFERENCES WITH L_ in 2.28
            LM[0,0]=LM[2,2]=dVxy[0]
            LM[0,1]=LM[2,3]=dVxy[1]
            LM[1,0]=LM[3,2]=dVxy[2]
            LM[1,1]=LM[3,3]=dVxy[3]
           
            
            #BL interpolators BLijk (4,4,8) (B.17 p165)
            for k in range(8):
                BL[0,0,k]=BL[2,2,k]=Bv[0,k]
                BL[0,1,k]=BL[2,3,k]=Bv[2,k]
                BL[1,0,k]=BL[3,2,k]=Bv[1,k]
                BL[1,1,k]=BL[3,3,k]=Bv[3,k]
                BL[0,2,k]=BL[0,3,k]=BL[1,2,k]=BL[1,3,k]=BL[2,0,k]=BL[2,1,k]=BL[3,0,k]=BL[3,1,k]=0.
            #Set tractions tP (2.34)
            
            
                
            w=1. #TO MODIFY
            #Calculate sigma
            #2.31 Comes first from 2.2 (Then 2.31)
            #Pij=vk d(sig ij)/xk - dvi/dxk sig(kj) + dvk/dxk sig ij
            #Calculate Piola Kirchoff Pi (2.31) Gij Cjk˙¯-Gij LM (jk) sig(k) + 
            #Attention double contraction
            #P=G*c*E-G*LM*sig+(LM[0,0]+LM[1,1])*G*sig
            
            #Calculate stabilization parameter
            tau=1.
            #STRESSES**********
            #From 2.27 Plane Strain Symmetric tensors are defined as 
            #t=[txx tyy tzz tyz]
            pi=1./3.*(sig[0,0]+sig[1,0]+sig[2,0])
            for i in range(3): #Only daigonal is modified
                sig_d[i,0]=sig[i,0]-pi #comps are [x y z yz]
            print ("sigd",sig_d[i][0])   
            for k in range(4):
                sig_eq=sqrt(1.5*(sig_d[k,0]))
            #*** STRAINS
            #Equivalent strain rate
            mat_A=mat_A0*math.exp(-mat_Q/mat_R)
            #Equivalent viscoplastic strain rate
            epsr_vp_eq=0.
            if (s!=0):
                epsr_vp_eq=mat_A*(sinh(mat_psi*sig_eq/s))**(1./mat_m)
            
            #print (epsr_vp_eq)
            #Evaluate g function 2.58/4.48
            g_sigs=1.
            #g_sigs=h0*|(1-s/s*)|*sign(1-s/s*)A(sinh())
            #With s*
            #s*=s~(edot~_vp/A)^n
            #Direction of plastic Strain: 2.16
            N_d=sqrt(3./2.)*sig_d/sig_eq
            Dvp=sqrt(3./2.)*epsr_vp_eq*N_d
            #print ("Nd",N_d)
            #With evp = f(sigma,s) (Sec. 2.4.2)
            #eps_vp=A sinh (psi (sigma/s))^(1/m) (2.57 p27)
            #Calculate Rate of Def Tensor D (2.13, 2.14)
            #D(th)ij=alpha vk dT/dxk deltaij 2.13
            #IN THIS EXAMPLE THIS DTH 0
            #D(vp)ij=sqrt(3/2) e. vp Nij  2.14
            #Nij Direction of plastic flow
            #Assemble DM matrix 4.24 p92
            if form==2:
                #for i in range(2):
                #    for j in range(2):
                #Dvp comes from N, which comes from 
                #Dvp is [x y z yz]
                DM[0,0]=DM[2,2]=Dvp[0]
                DM[1,1]=DM[3,3]=Dvp[1]
                DM[1,0]=DM[3,2]=DM[0,1]=DM[2,3]=Dvp[3]
                DM[3,3]=Dvp[2]
            
            wJ=w*detJ
            
            #Elastic part of F 4.6
            #ATENTION F~ is not in the same order of NODAL variable 
            if (it==0):
                Ft=identity(3)
                #NOT USE!!! Fd=[[1,0,0,1]]
                Fvpt=identity(3)
                print(Ft)
                Fvpd[0]=1.
                Fvpd[1]=0. #yx
                Fvpd[2]=0. #xy
                Fvpd[3]=1. 
                Fvpd[4]=1.
            else:
                Ft[0,0]=F[0]
                Ft[0,1]=F[1]
                Ft[1,0]=F[2]
                Ft[1,1]=F[3]
                Ft[2,2]=1.
                #F is [xx xy yx yy zz]
                Fd[0]=F[0]
                Fd[1]=F[2] #yx
                Fd[2]=F[1] #xy
                Fd[3]=F[3]
                #F is [xx xy yx yy zz]
                Fvpd[0]=Fvp[0]
                Fvpd[1]=Fvp[2] #yx
                Fvpd[2]=Fvp[1] #xy
                Fvpd[3]=Fvp[3]  
                
            visc=1.
          
            #print("Fd",Fd[0,0])
            #Arguments passed are ~ vectors
            dEdU=deriv.calc_dEdU(Fd,Fvpd,NsigF,NFvp)
            
            # *****************************************************************
            #RESIDUALS ******************* 2.26 to 2.39 *****************************
            Rv  =Bv.transpose()*P #Remains the summ of particular gauss points
            #Rsig[16x1] (4 per node)
            #Construct vk Bsig mik
            for m in range(4):
                for i in range(16):
                        temp4x16[m,i]=0.
            for m in range(4):
                for i in range(16):
                    for k in range(2):
                        temp4x16[m,i]=temp4x16[m,i]+BsigF[m,i,k]*v[k,0]
            
            if form==2:
                for m in range(5):
                    for i in range(20):
                        for k in range(2):
                            temp5x16[m,i]=temp5x16[m,i]+BFvp[m,i,k]*v[k,0]
                        
            if (form==1):
                Rsig=(NsigF+temp4x16*tau).transpose()*(temp4x16*Usig-c*Ee)*wJ
            else: #4.29
                RFvp=(NFvp+temp5x16*tau).transpose()*(temp5x16*UFvp-DM*NFvp*UFvp)*wJ
            RF  =(NsigF+temp4x16*tau).transpose()*(temp4x16*UF-LM*NsigF*UF)*wJ
            Rs  =(Ns+tau*v.transpose()*Bs).transpose()*(v.transpose()*Bs*Us-g_sigs)*wJ
            
            
            #R Assembly            
            #TANGENT MATRIX   
            #PAGES 25 y 94
            #dRdUn=Bv.transpose()*( G*c*dEdU -G*)
            # for j,n in range(8,8):
                # for i in range(8,8):
                # Kt[0][0]=   Kt[0][0]+B[i][j]*(
                            # #*G[i][p]*C[p][k]
                            # -G[i][l]*BL[l][k][n]*sigma[k]
                            # +G[i][k]*sigma[k]*H[m][l]*BL[m][l][n]*
                            # wJ)   
            #temp8x1=0
            #*** ONLY FOR FORMULATION 2
            # for l,m,n in range(4,4,8):
                # temp8x1[l]=temp8x1[l]+BL[m][l][n]
            
            # #print("temp",temp8x1[0])  
            # Kt[0][0]=   Kt[0][0]+Bv.transpose()*(
                        # #G[i][p]*C[p][k]
                        # #-G[i][l]*BL[l][k][n]*sigma[k]
                        # +G*sig*temp8x1.transpose()*
                        # wJ
            #dRv/dUv
            Kt[0][0]=   Kt[0][0]+Bv.transpose()*visc*Bv*wJ
            
            #dRv/dUF
            #(8x16)
            Kt[0][1]=   Kt[0][1]+Bv.transpose()*c*dEdU[0]*wJ
            #dRv/dUF (8x20)
            Kt[0][2]=   Kt[0][2]+Bv.transpose()*c*dEdU[1]*wJ
            #Kt(0,3) dRv/dUs=0
            
            #----------------------------------------------------
            #for i in range(4):
            #Kt.clear()
            #F derivatives (2.49 to 2.52)
            if (form==1):
                find=int(2)
            else:
                find=int(1)
                
            for m in range(4):
                for j in range(16):
                    for k in range(2):
                        temp4x2[m,k]=temp4x2[m,k]+BsigF[m,j,k]*UF[j,0]

            for m,l,p in zip(range(4),range(4),range(8)):
                temp4x8[m,p]=BL[m,l,p]+F[l,0]            
            #dRF/dUv 4.35
            #(16x8)
            #print("Nv",Nv)
            Kt[find][0]   =Kt[find][0]+( 
                             (NsigF.transpose()*temp4x2*Nv) #16x4*4x2*2x8
                            #+tau*N
                            -(NsigF+float(tau)*temp4x16).transpose()*temp4x8
                            )*wJ
            #dRFdUF  4.36
            Kt[find][find]= Kt[find][find]+(
                            (NsigF+temp4x16*tau).transpose()*
                            (temp4x16-LM*NsigF)
                            )*wJ
                            
            #dRFdUF=dRFdUF=0.  4.37 & 4.38
            
            #---------------------------------------------------
            ##Viscoplastic derivatives
            #Kt(2,0)=dFvp/dUV 4.39
            temp4x1=LM*NsigF*UF
            
            for m,i,n in zip(range(5),range(20),range(2)):
                temp1=temp2=0. #increase with each m
                for j,k in zip(range(20),range(2)):
                    temp1=temp1+2*BFvp[m,j,k]*v[k]*UFvp[j,0]
                temp20x2[i,n]=  temp20x2[i,n]+BFvp[m,i,n]*(temp1-temp4x1[m,0])
            
            #print("size",len((NFvp+float(tau)*temp5x16)))
            Kt[2][0]=Kt[2][0]+(
                        #NFvp.transpose()*
                         tau*temp20x2*Nv
                        -(NFvp+float(tau)*temp5x16).transpose()*temp_dDFvp[0]
                        )*wJ
            
            #dRFvp/dUF 4.42
            Kt[2][1]=Kt[2][1]-(
                         (NFvp+float(tau)*temp5x16).transpose()*temp_dDFvp[1]
                        )*wJ
            
            #dRFvp/dUFvp 4.41
            Kt[2][2]=Kt[2][2]-(
                         (NFvp+float(tau)*temp5x16).transpose()*(
                         temp5x16-DM*NFvp-temp_dDFvp[2])
                        )*wJ
                        
            #---------------------------------------------------
            #S derivatives -- COMMON TO BOTH FORMULATIONS
            #2.53 and 4.xx ATENTION IN CHAPTER 4 k and p indices are wrong, see 2.53
            #dRs/dUv
            # print("test",tau*Bs.transpose()*Nv*(2.*v*Bs*Us * g_sigs) 
            #print("test",float(2.0*(Bs*Us).transpose()*v))
            Kt[3][0]=Kt[3][0]+(
                      Ns.transpose()*(Bs*Us).transpose()*Nv +
                      tau*Bs.transpose()*Nv*float(2.*(Bs*Us).transpose()*v * g_sigs)
                      )*wJ 
            
            #4.44 dRs/dF 4.44 (4x16)
            # Kt[3][1]=Kt[3][1]+(
                      # (Ns+tau*v.transpose()*Bs).transpose()*
                      # (v.transpose()*Bs)*
                      # wJ) 
            
            #dRs/dFvp 4.45
            #4x20
            Kt[3][2]=Kt[3][2]+(
                    (Ns.transpose()+tau*Bs.transpose()*v)*(-1.)*dgdU[2]
                    )*wJ
            #dRs/dS 4.46    
            Kt[3][3]=Kt[3][3]+(
                      (Ns+tau*v.transpose()*Bs).transpose()*
                      (v.transpose()*Bs-dgdU[3])*
                      wJ) 

              # (Ns[i]*Bs[k][j]*Us[j]*Nv[k][p] +
              # tau*Bs[k][i]*Nv[k][p]*(2.*Bs[m][j]*Us[j]*v[m] * g_sigs))*
              # wJ) 
                     
            #Index Form
                # for i in range(16):
                    # for p in range(2):
                        # dRdUn=Bv.transpose()*( G*c*dEdU -G*)
            
            #print (B)
            #K+=(B.transpose()*c*B*w)
            #print (K)
    #print (K)

#Boundary conditions
#Numerated as in Bathe
# for i in range(8):
    # K[4,i] = K[i,4] = 0.0
    # K[5,i] = K[i,5] = 0.0
    # K[7,i] = K[i,7] = 0.0
	
# K[4,4] = K[5,5] = K[7,7] = 1.;
# R=[0,0,1000.0,0,0,0,0,0]

# #Numerated as in deal.ii
# for i in range(8):
    # K[0,i] = K[i,0] = 0.0
    # K[1,i] = K[i,1] = 0.0
    # K[3,i] = K[i,3] = 0.0
	
# K[0,0] = K[1,1] = K[3,3] = 1.;
# R=[0,0,0,0,1000.0,0,0,0]

#print (K)

#U=linalg.solve(K, R)
print ("Results")
#print(U)


