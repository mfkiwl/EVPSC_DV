#ifndef INPUT_H
#define INPUT_H

#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>

using namespace std;
using namespace Eigen;
using json = nlohmann::json;

#include "Polycrystals.h"
#include "Processes.h"

int EVPSCinput(string &,string &,string &, Procs::Process &); //read .in file
int sxinput(string, Polycs::polycrystal &); //read .sx file
int texinput(string, Polycs::polycrystal &); //read .tex file
int loadinput(string, Procs::Process &Proc);
VectorXd getnum(string, int);
// add more input functions here
vector<double> getnum_vec(string, int);
void add_trans_miller(string crysym, json &sx_json);
void add_lattice_const(VectorXd ccon, json &sx_json);
void add_elastic_constant(MatrixXd Cij6, json &sx_json);
void add_thermal_coefficient(VectorXd ther, json &sx_json);
vector<double> get_vector(MatrixXd &matrix);
vector<double> get_vector(VectorXd &matrix);
MatrixXd cal_sn_info(MatrixXd &Min, vector<double> Mabc, vector<double> Trans_Miller, int Miller_n, int system_n);
json sx_info_postprocess(json &sx_json);
double cal_shear_modulus(vector<double> Cij6, vector<double> sn);
#endif
