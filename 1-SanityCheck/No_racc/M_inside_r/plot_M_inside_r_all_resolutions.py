#!/usr/bin/env python
import matplotlib.pyplot as pl
import numpy as np

def load_data(filename):
	all_lines = []
	with open(filename) as f:
		for line in f:
			if (line[0] == '#'):
				continue
			#for char in line:
			line_data = np.array(line.split(),dtype=float)
			all_lines.append(line_data)
	return all_lines

def M_in(r):
	Mbh = 1e-2
	Mcore = 2e-2
	Mbulge = 1.0
	rcore = 0.02 
	rbulge = 1.0
	if( r < rcore ):
		return Mbh + Mcore * (r/rcore)**3
	else:
		return Mbh + Mbulge * (r/rbulge)

def Jmom(r):
	return np.sqrt( r * M_in( r ))

#Set relevant dimensions:
FigSize = 10
FontSize = 20
BorderWidth = 3
pl.rcParams.update({'font.size': FontSize})
pl.rcParams.update({'axes.linewidth': BorderWidth})
#Open figure:
pl.figure( figsize=( FigSize, FigSize ) )
pl.tick_params(width=BorderWidth, length=FontSize, which='major')
pl.tick_params(width=BorderWidth, length=0.3*FontSize, which='minor')


snap=5
#print("#M_bh r_circ")
for res in ['100k','250k','500k','750k','001M']:
	datafile = "./res_"+res+"_t"+str(snap)+"00.txt"
	X = np.array(load_data(datafile))
	M_cumulative = np.array(X[:,1])
	print('#'+res)
	for i in range(1,len(M_cumulative)):
		M_cumulative[i] += M_cumulative[i-1]
		print(X[i,0], M_cumulative[i])
	label_this= res#+" particles, $t = "+str(snap)+"$"
	imgplot = pl.loglog(X[:,0], M_cumulative,'-', linewidth = 0.5*FontSize, label = label_this)

#plot expectation: From generalized eq 12:
#M(<r*) / Mshell ~
r_fin = np.linspace(1e-4,0.2)
J_rfin = np.zeros(len(r_fin))
for i in range(len(r_fin)):
	J_rfin[i] = Jmom(r_fin[i])

pl.plot(r_fin, J_rfin**2, 'k--', label='expected')
pl.plot(r_fin, r_fin, 'k.', label='expected')
#ONLY VALID FOR r << r_core = 0.02


pl.legend(loc=2)
pl.xlabel("$r$",fontsize=1.5*FontSize)
pl.ylabel("$N_{\\rm part}$",fontsize=1.5*FontSize)
pl.xlim([1e-4,2e-1])
pl.ylim([1e-6,1e0])
#pl.title("Circularization radius is where the peak is..." )
figfile = "M_inside_r_all_res.png"
pl.savefig(figfile)
print("Figure saved to "+figfile)
#pl.show()
