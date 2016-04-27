#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <gsl/gsl_math.h>

#include "allvars.h"
#include "proto.h"

#ifdef COSMIC_RAYS
#include "cosmic_rays.h"
#endif

/*! \file predict.c
 *  \brief drift particles by a small time interval
 *
 *  This function contains code to implement a drift operation on all the
 *  particles, which represents one part of the leapfrog integration scheme.
 */


/*! This function drifts all particles from the current time to the future: time0 - > time1
 *
 *  If there is no explicit tree construction in the following timestep, the
 *  tree nodes are also drifted and updated accordingly. For periodic boundary
 *  conditions, the mapping of coordinates onto the interval [0,All.BoxSize]
 *  is only done before the domain decomposition, or for outputs to snapshot
 *  files.  This simplifies dynamic tree updates, and allows the domain
 *  decomposition to be carried out only every once in a while.
 */
void move_particles(int time0, int time1)
{
  int i, j;
  double dt_drift, dt_gravkick, dt_hydrokick, dt_entr;
  double t0, t1;

#ifdef XXLINFO
#ifdef MAGNETIC
  double MeanB_part = 0, MeanB_sum;

#ifdef TRACEDIVB
  double MaxDivB_part = 0, MaxDivB_all;
  double dmax1, dmax2;
#endif
#endif
#ifdef TIME_DEP_ART_VISC
  double MeanAlpha_part = 0, MeanAlpha_sum;
#endif
#endif

#ifdef SOFTEREQS
  double a3inv;

  if(All.ComovingIntegrationOn)
    a3inv = 1 / (All.Time * All.Time * All.Time);
  else
    a3inv = 1;
#endif

  t0 = second();

  if(All.ComovingIntegrationOn)
    {
      dt_drift = get_drift_factor(time0, time1);
      dt_gravkick = get_gravkick_factor(time0, time1);
      dt_hydrokick = get_hydrokick_factor(time0, time1);
    }
  else
    {
      dt_drift = dt_gravkick = dt_hydrokick = (time1 - time0) * All.Timebase_interval;
    }

  for(i = 0; i < NumPart; i++)
    {
      for(j = 0; j < 3; j++)
	P[i].Pos[j] += P[i].Vel[j] * dt_drift;

#ifndef HPM
      if(P[i].Type == 0)
	{
#ifdef PMGRID
	  for(j = 0; j < 3; j++)
	    SphP[i].VelPred[j] +=
	      (P[i].g.GravAccel[j] + P[i].GravPM[j]) * dt_gravkick + SphP[i].a.HydroAccel[j] * dt_hydrokick;
#else
	  for(j = 0; j < 3; j++)
	    SphP[i].VelPred[j] += P[i].g.GravAccel[j] * dt_gravkick + SphP[i].a.HydroAccel[j] * dt_hydrokick;
#endif
	  SphP[i].a2.Density *= exp(-SphP[i].u.s.a4.DivVel * dt_drift);
	  PPP[i].Hsml *= exp(0.333333333333 * SphP[i].u.s.a4.DivVel * dt_drift);

	  if(PPP[i].Hsml < All.MinGasHsml)
	    PPP[i].Hsml = All.MinGasHsml;

	  dt_entr = (time1 - (P[i].Ti_begstep + P[i].Ti_endstep) / 2) * All.Timebase_interval;

#ifndef MHM
#ifndef SOFTEREQS
	  SphP[i].Pressure =
	    (SphP[i].Entropy + SphP[i].e.DtEntropy * dt_entr) * pow(SphP[i].a2.Density, GAMMA);
#else
	  if(SphP[i].a2.Density * a3inv >= All.PhysDensThresh)
	    SphP[i].Pressure =
	      All.FactorForSofterEQS * (SphP[i].Entropy +
					SphP[i].e.DtEntropy * dt_entr) * pow(SphP[i].a2.Density,
									     GAMMA) + (1 -
										       All.
										       FactorForSofterEQS) *
	      GAMMA_MINUS1 * SphP[i].a2.Density * All.InitGasU;
	  else
	    SphP[i].Pressure =
	      (SphP[i].Entropy + SphP[i].e.DtEntropy * dt_entr) * pow(SphP[i].a2.Density, GAMMA);
#endif
#else
	  /* Here we use an isothermal equation of state */
	  SphP[i].Pressure = GAMMA_MINUS1 * SphP[i].a2.Density * All.InitGasU;
	  SphP[i].Entropy = SphP[i].Pressure / pow(SphP[i].a2.Density, GAMMA);
#endif

#ifdef COSMIC_RAYS
#if ( defined( CR_UPDATE_PARANOIA ) )
	  CR_Particle_Update(SphP + i);
#endif
#ifndef CR_NOPRESSURE
	  SphP[i].Pressure += CR_Comoving_Pressure(SphP + i);
#endif
#endif


#ifdef MAGNETIC
	  for(j = 0; j < 3; j++)
	    SphP[i].BPred[j] += SphP[i].DtB[j] * dt_entr;
#ifdef DIVBCLEANING_DEDNER
	  SphP[i].PhiPred += SphP[i].DtPhi * dt_entr;
#endif
#endif
#ifdef XXLINFO
	  if(Flag_FullStep == 1)
	    {
#ifdef MAGNETIC
	      MeanB_part += sqrt(SphP[i].BPred[0] * SphP[i].BPred[0] +
				 SphP[i].BPred[1] * SphP[i].BPred[1] + SphP[i].BPred[2] * SphP[i].BPred[2]);
#ifdef TRACEDIVB
	      MaxDivB_part = DMAX(MaxDivB, fabs(SphP[i].divB));
#endif
#endif
#ifdef TIME_DEP_ART_VISC
	      MeanAlpha_part += SphP[i].alpha;
#endif
	    }
#endif
	}
#endif /* end of HPM */
    }


  t1 = second();
  All.CPU_Predict += timediff(t0, t1);
  All.Cadj_Cpu += timediff(t0, t1);
  CPU_Step[CPU_MOVE] += timediff(t0, t1);



#ifdef XXLINFO
  if(Flag_FullStep == 1)
    {
#ifdef MAGNETIC
      MPI_Reduce(&MeanB_part, &MeanB_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
      if(ThisTask == 0)
	MeanB = MeanB_sum / All.TotN_gas;
#ifdef TRACEDIVB
      MPI_Reduce(&MaxDivB_part, &MaxDivB_all, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(ThisTask == 0)
	MaxDivB = MaxDivB_all;
#endif
#endif
#ifdef TIME_DEP_ART_VISC
      MPI_Reduce(&MeanAlpha_part, &MeanAlpha_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
      if(ThisTask == 0)
	MeanAlpha = MeanAlpha_sum / All.TotN_gas;
#endif
    }
#endif
}



/*! This function makes sure that all particle coordinates (Pos) are
 *  periodically mapped onto the interval [0, BoxSize].  After this function
 *  has been called, a new domain decomposition should be done, which will
 *  also force a new tree construction.
 */
#ifdef PERIODIC
void do_box_wrapping(void)
{
  int i, j;
  double boxsize[3];

  for(j = 0; j < 3; j++)
    boxsize[j] = All.BoxSize;

#ifdef LONG_X
  boxsize[0] *= LONG_X;
#endif
#ifdef LONG_Y
  boxsize[1] *= LONG_Y;
#endif
#ifdef LONG_Z
  boxsize[2] *= LONG_Z;
#endif

  for(i = 0; i < NumPart; i++)
    for(j = 0; j < 3; j++)
      {
	while(P[i].Pos[j] < 0)
	  P[i].Pos[j] += boxsize[j];

	while(P[i].Pos[j] >= boxsize[j])
	  P[i].Pos[j] -= boxsize[j];
      }
}
#endif
