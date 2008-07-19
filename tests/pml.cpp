#include <stdio.h>
#include <stdlib.h>

#include <meep.hpp>
using namespace meep;

double one(const vec &) { return 1.0; }

static complex<double> do_ft(fields &f, component c, const vec &pt, double freq)
{
  complex<double> ft = 0.0;
  double emax = 0;
  while (f.time() < f.last_source_time()) {
    complex <double> fpt = f.get_field(c, pt);
    ft += fpt * polar(1.0, 2*pi*freq * f.time());
    emax = max(emax, abs(fpt));
    f.step();
  }
  do {
    double emaxcur = 0;
    double T = f.time() + 50;
    while (f.time() < T) {
      complex <double> fpt = f.get_field(c, pt);
      ft += fpt * polar(1.0, 2*pi*freq * f.time());
      double e = abs(fpt);
      emax = max(emax, e);
      emaxcur = max(emaxcur, e);
      f.step();
    }
    if (emaxcur < 1e-6 * emax) break;
  } while(1);
  return ft;
}

int check_pml1d(double eps(const vec &)) {
  double freq = 1.0, dpml = 1.0;
  double sz = 10.0 + 2*dpml;
  double sz2 = 10.0 + 2*dpml*2;
  complex<double> ft = 0.0, ft2 = 0.0;
  double prev_refl_const = 0.0, refl_const = 0.0;
  vec fpt(0.5*sz - dpml - 0.1);
  master_printf("Checking resolution convergence of 1d PML...\n");
  for (int i=0; i<8; i++) {
    double res = 10.0 + 10.0*i;
    {
      volume v = vol1d(sz,res);
      v.center_origin();
      structure s(v, eps, pml(dpml));
      fields f(&s);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(Ex, src, vec(-0.5*sz+dpml+0.1));
      ft = do_ft(f, Ex, fpt, freq);
    }
    {
      volume v = vol1d(sz2,res);
      v.center_origin();
      structure s(v, eps, pml(dpml*2));
      fields f(&s);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(Ex, src, vec(-0.5*sz+dpml+0.1));
      ft2 = do_ft(f, Ex, fpt, freq);
    }
    refl_const = pow(abs(ft - ft2),2.0) / pow(abs(ft2),2.0);
    master_printf("refl1d:, %g, %g\n", res, refl_const);
    //    if (i > 1 && 
    //	refl_const > prev_refl_const * pow((res - 10)/res,8.0) * 1.1)
    //   return 1;
    prev_refl_const = refl_const;
  }
  master_printf("passed 1d PML check.\n");
  return 0;
}

int check_pml2d(double eps(const vec &), component c) {
  double freq = 1.0, dpml = 1.0;
  complex<double> ft = 0.0, ft2 = 0.0;
  double prev_refl_const = 0.0, refl_const = 0.0;
  double sxy = 5.0 + 2*dpml;
  double sxy2 = 5.0 + 2*dpml*2;
  double res_step = 6.0;
  vec fpt(0.5*sxy - dpml - 0.1,0);
  if (c != Ez && c != Hz) abort("unimplemented component check");
  double symsign = c == Ez ? 1.0 : -1.0;
  master_printf("Checking resolution convergence of 2d %s PML...\n",
		c == Ez ? "TM" : "TE");
  for (int i=0; i<4; i++) {
    double res = 10.0 + res_step*i;
    {
      volume v = vol2d(sxy,sxy,res);
      v.center_origin();
      const symmetry S = mirror(X,v)*symsign + mirror(Y,v)*symsign;
      structure s(v, eps, pml(dpml), S);
      fields f(&s);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(c, src, v.center());
      ft = do_ft(f, c, fpt, freq);
    }
    {
      volume v = vol2d(sxy2,sxy2,res);
      v.center_origin();
      const symmetry S = mirror(X,v)*symsign + mirror(Y,v)*symsign;
      structure s(v, eps, pml(dpml*2), S);
      fields f(&s);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(c, src, v.center());
      ft2 = do_ft(f, c, fpt, freq);
    }
    refl_const = pow(abs(ft - ft2),2.0) / pow(abs(ft2),2.0);
    master_printf("refl2d:, %g, %g\n", res, refl_const);
    if (i > 1 && 
    	refl_const > prev_refl_const * pow((res - res_step)/res,8.0) * 1.1)
      return 1;
    prev_refl_const = refl_const;
  }
  master_printf("passed 2d %s PML check.\n", c == Ez ? "TM" : "TE");
  return 0;
}

/* The cylindrical case actually shouldn't have a reflection that goes
   to zero with increasing resolution - we implement only a
   "quasi-PML" for cylindrical coordinates, which is basically the PML
   for Cartesian coordinates copied over directly to the cylindrical
   case, rather than doing a proper coordinate stretching of r.  This
   is not a practical issue because, rather than increasing the
   resolution, in practice you increase the PML thickness to eliminate
   reflections, and increasing a quasi-PML thickness makes the
   reflection vanish by the usual adiabatic theorem. 

   Because of that, we don't actually run this check as part of the
   test suite, but I'll leave the code here for future study of the 
   cylindrical PML. */
int check_pmlcyl(double eps(const vec &)) {
  double freq = 1.0, dpml = 1.0;
  complex<double> ft = 0.0, ft2 = 0.0;
  double prev_refl_const = 0.0, refl_const = 0.0;
  double sr = 5.0 + dpml, sz = 1.0 + 2*dpml;
  double sr2 = 5.0 + dpml*2, sz2 = 1.0 + 2*dpml*2;
  vec fpt = veccyl(sr - dpml - 0.1,0);
  double res_step = 6.0;
  master_printf("Checking resolution convergence of cylindrical PML...\n");
  for (int i=0; i<5; i++) {
    double res = 10.0 + res_step*i;
    master_printf("    checking cylindrical resolution %g...\n", res);
    {
      volume v = volcyl(sr,sz,res);
      v.center_origin();
      structure s(v, eps, pml(dpml));
      fields f(&s, 0);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(Ez, src, veccyl(0.1,0.1));
      ft = do_ft(f, Ez, fpt, freq);
    }
    {
      volume v = volcyl(sr2,sz2,res);
      v.center_origin();
      structure s(v, eps, pml(dpml*2));
      fields f(&s, 0);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(Ez, src, veccyl(0.1,0.1));
      ft2 = do_ft(f, Ez, fpt, freq);
    }
    refl_const = pow(abs(ft - ft2),2.0) / pow(abs(ft2),2.0);
    master_printf("reflcyl:, %g, %g\n", res, refl_const);
    prev_refl_const = refl_const;
  }
  master_printf("passed cylindrical PML check.\n");
  return 0;
}

int pml1d_scaling(double eps(const vec &)) {
  double res = 20, freq = 1.0, dpml = 0;
  complex<double> prev_ft = 0.0, ft = 0.0;
  double refl_const = 0.0;
  master_printf("Checking thickness convergence of 1d PML...\n");
  for (int i=0; i<7; i++) {
    dpml = pow(2.0,(double)i);
    double sz = 2*dpml + 10.0 + dpml;
    prev_ft = ft;
    {
      volume v = vol1d(sz,res);
      structure s(v, eps, (pml(2*dpml,Z,Low) + pml(dpml,Z,High)) * 1.5);
      fields f(&s);
      gaussian_src_time src(freq, freq / 20);
      f.add_point_source(Ex, src, vec(2*dpml+0.1));
      ft = do_ft(f, Ex, vec(sz - dpml - 0.1), freq);
    }
    if (i > 0) {
      refl_const = pow(abs(ft - prev_ft),2.0) / pow(abs(prev_ft),2.0);
      master_printf("refl1d:, %g, %g\n", dpml / 2.0, refl_const);
      if (refl_const > (1e-9)*pow(2/dpml,6.0)
	  || refl_const < (1e-10)*pow(2/dpml, 6.0)) return 1;
    }      
  }
  master_printf("pml scales correctly with length.\n");
  return 0;
}

int main(int argc, char **argv) {
  initialize mpi(argc, argv);
  quiet = true;
  master_printf("Running PML tests...\n");
  if (check_pml1d(one)) abort("not a pml in 1d.");
  if (check_pml2d(one,Ez)) abort("not a pml in 2d TM.");
  if (check_pml2d(one,Hz)) abort("not a pml in 2d TE."); 
  // if (check_pmlcyl(one)) abort("not a pml in cylincrical co-ordinates.");
  if (pml1d_scaling(one)) abort("pml doesn't scale properly with length.");
  return 0;
}