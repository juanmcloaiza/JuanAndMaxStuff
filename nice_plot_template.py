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

datafile = "rcirc_vs_mbh.txt"
print("reading " + datafile)
X = np.array(load_data(datafile))

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

label_this="Analytical: ${l_0} = 0.018$"
imgplot = pl.loglog(X[:,0], X[:,2], '-', linewidth = 0.5*FontSize, label = label_this)

label_this="Simulations: ${l_0} = 0.018$"
imgplot = pl.loglog(X[:,0], X[:,1], 'ro', markersize = FontSize, label = label_this)

pl.legend(loc=1)
pl.xlabel("${\\rm M_{BH}}$",fontsize=30)
pl.ylabel("$r_{\\rm circ}$",fontsize=30)
#pl.xlim([0,1])
#pl.ylim([0,1])
#pl.title("Circularization radius as function of black hole mass")
figfile = "rcirc_mbh.png"
pl.savefig(figfile)
print("Figure saved to "+figfile)
pl.close()
#pl.show()
