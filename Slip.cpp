#include "Slip.h"
#include "global.h"
#include <string>

Slip::Slip() {};

Slip::Slip(json &j_slip)
{
    logger.info("Slip system: " + to_string(j_slip["id"]));
    mtype = j_slip["type"];  num = j_slip["id"];  shear_modulus = j_slip["G"];
    logger.info("Shear modulus: " + to_string(shear_modulus));
    strain_rate_slip = 0; drate_dtau = 0; acc_strain = 0; disloc_velocity = 0; 

    VectorXd info_vec = to_vector(j_slip, "sn", 6);
    plane_norm = info_vec(seq(0,2)); //normal 
    burgers_vec = info_vec(seq(3,5)); //Burgers
    Pij = 0.5 * (burgers_vec / burgers_vec.norm() * plane_norm.transpose() + plane_norm * burgers_vec.transpose() / burgers_vec.norm());
    Rij = 0.5 * (burgers_vec * plane_norm.transpose() / burgers_vec.norm() - plane_norm * burgers_vec.transpose() / burgers_vec.norm());

    rate_sen = j_slip["nrsx"];
    int len = j_slip["CRSS_p"].size();
    if (len == 4) flag_harden = 0; else flag_harden = 1;
    for (int i = 0; i != len; ++i) harden_params.push_back(j_slip["CRSS_p"][i]); 
    for (int i = 0; i != j_slip["hst"].size(); ++i) latent_params.push_back(j_slip["hst"][i]);
    for (int i = 0; i != 5; ++i) update_params.push_back(0);
    if (flag_harden == 0) crss = harden_params[0];
    else {
        disloc_density = j_slip["CRSS_p"][0];
        rho_mov = disloc_density;
        update_params[0] = burgers_vec.norm() * 1e-10;
    }
}

int Slip::ini_sn_mode(VectorXd matrix_input, int mode_type, int system_n)
{
    mtype = mode_type;
    flag_harden = 1;
    num = system_n;
    VectorXd info_vec = matrix_input;
    plane_norm = info_vec(seq(0,2)); //normal 
    plane_norm = plane_norm/plane_norm.norm();
    burgers_vec = info_vec(seq(3,5)); //Burgers
    Pij=0.5*(burgers_vec/burgers_vec.norm()*plane_norm.transpose()+plane_norm*burgers_vec.transpose()/burgers_vec.norm());
    Rij=0.5*(burgers_vec*plane_norm.transpose()/burgers_vec.norm()-plane_norm*burgers_vec.transpose()/burgers_vec.norm());
    strain_rate_slip = 0; drate_dtau = 0; shear_modulus = 0; acc_strain = 0; disloc_velocity = 0; 
    return 0;
}

int Slip::check_sn_mode()
{
    if(mtype == 1)
    {
        logger.info("Slip system: " + to_string(num));
        logger.info("Normal: ");
        logger.info(plane_norm.transpose());
        logger.info("Burgers: ");
        logger.info(burgers_vec.transpose());
        logger.info("Pij: ");
        logger.info(Pij);
    }
    if(mtype == 0)
    {
        logger.info("Twinning system: " + to_string(num));
        logger.info("Normal: ");
        logger.info(plane_norm.transpose());
        logger.info("Shear direction: ");
        logger.info(burgers_vec.transpose());
        logger.info("Pij: ");
        logger.info(Pij);
    }
    return 0;
}

int Slip::ini_hardening_mode(double nrsx_in, VectorXd hardens_in, VectorXd latents_in)
{
    /*
     * harden parameters: 0: SSD_density,
     * 1: freq_Debye, 2: c_length, 3: kink_energy_ref, 4: temperature_ref,
     * 5: Peierls_stress, 6: expo_kinkeng, 7: wave_speed, 8: c_drag, 9: c_backstress,
     * 10: c_multi, 11: D, 12: ref_srate, 13: gg;
     * 
     * update parameters:
     * 0: burgers, 1: disl_density_for, 2: disl_density_resist, 3: back_stress,
     * 4: barrier_distance, 5:c_annih;
     */
    rate_sen = nrsx_in;
    disloc_density = hardens_in(0);
    for (int i = 0; i != hardens_in.size(); ++i) harden_params.push_back(hardens_in(i)); 
    for (int i = 0; i != latents_in.size(); ++i) latent_params.push_back(latents_in(i));
    if(harden_params.size() != 4) flag_harden = 0;
    else flag_harden = 1;
    for (int i = 0; i != 5; ++i) update_params.push_back(0);
    update_params[0] = burgers_vec.norm() * 1e-10;
    return 0;
}

int Slip::check_hardening_mode()
{
    logger.info("Slip system: " + to_string(num));
    string str = "";
    for (int i = 0; i != harden_params.size(); ++i) str += std::to_string(harden_params[i]) + "\t";
    logger.info("Hardening parameters: " + str);
    return 0;    
}

double Slip::get_gamma0(){ return ref_rate; }
double Slip::get_nrsx(){ return rate_sen; }

double Slip::cal_rss(Matrix3d stress_tensor){ return (stress_tensor.cwiseProduct(Pij)).sum();}
double Slip::cal_relative_rss(Matrix3d stress_tensor){ return (stress_tensor.cwiseProduct(Pij)).sum() / crss;}

Matrix3d Slip::cal_dijpmode(Matrix3d sig){ 
    cal_strain_rate(sig);
    return strain_rate_slip * Pij;}
Matrix3d Slip::cal_rotslip_m(){ return strain_rate_slip * Rij;}

Matrix6d Slip::get_Fgradm(Matrix3d stress_grain)
{
    cal_drate_dtau(stress_grain);
    Vector6d Pijv = voigt(Pij);
    /* logger.debug("Mode number: " + to_string(num) + " drate_dtau: " + to_string(drate_dtau)); */
    return Pijv * Pijv.transpose() * drate_dtau;
}

double Slip::update_shear_strain_m()
{
    gamma_rate_abs_m = abs(strain_rate_slip);
    return gamma_rate_abs_m;
}

// add function from SXCpp
void Slip::cal_strain_rate(Matrix3d stress_tensor){
    /* Select different model for shear strain rate, controlled by flag_harden.
     * 0 : Voce Hardening; 1 : Dislocation Velocity Model;
     * Power model will be used in case 0, while Velocity model used in case 1;
     */
    switch (flag_harden)
    {
        case 0:
            cal_strain_rate_pow(stress_tensor);
            break;
        case 1:
            cal_strain_rate_disvel(stress_tensor);
            break;
        default:
            cal_strain_rate_disvel(stress_tensor);
            break;
    }
}

void Slip::cal_strain_rate_pow(Matrix3d stress_tensor){
    double rss_slip = cal_rss(stress_tensor);       
    if(abs(rss_slip) > 0.5 * crss){
        strain_rate_slip = ref_rate * pow(abs(rss_slip / crss), 1/rate_sen)* sign(rss_slip); 
    }
    rss = rss_slip;
}

void Slip::cal_strain_rate_disvel(Matrix3d stress_tensor){
    double burgers = update_params[0];
    double rss_slip = cal_rss(stress_tensor);
    double disl_vel = disl_velocity(rss_slip);
    strain_rate_slip = abs(rho_mov * burgers * disl_vel) * sign(rss_slip);
    rss = rss_slip;
}

void Slip::cal_drate_dtau(Matrix3d stress_tensor){
    /* Select different model for the gradient of shear strain rate by rss, controlled by flag_harden.
     * Note the slip rate will also be update in this function.
     * 0 : Voce Hardening; 1 : Dislocation Velocity Model;
     * Power model will be used in case 0, while Velocity model used in case 1;
     */
    switch (flag_harden)
    {
        case 0:
            cal_drate_dtau_pow(stress_tensor);
            break;
        case 1:
            cal_drate_dtau_disvel(stress_tensor);
            break;
        default:
            cal_drate_dtau_disvel(stress_tensor);
            break;
    }
}

void Slip::cal_drate_dtau_pow(Matrix3d stress_tensor){
    double rss_slip = cal_rss(stress_tensor);       
    if(abs(rss_slip) > 0.5 * crss){
        drate_dtau = ref_strain_rate * pow(abs(rss_slip / crss), 1/rate_sen-1) * sign(rss_slip) / rate_sen / crss * sign(rss_slip); 
        strain_rate_slip = ref_strain_rate * pow(abs(rss_slip / crss), 1/rate_sen) * sign(rss_slip); 
    }
    rss = rss_slip;
}   

void Slip::cal_drate_dtau_disvel(Matrix3d stress_tensor){
    double burgers = update_params[0];
    double rss_slip = cal_rss(stress_tensor);
    vector<double> dvel_and_vel = disl_velocity_grad(rss_slip);
    drate_dtau = rho_mov * burgers * sign(rss_slip) * dvel_and_vel[0];
    strain_rate_slip = rho_mov * burgers * dvel_and_vel[1] * sign(rss_slip);
    rss = rss_slip;
}

void Slip::update_status(grain &gr_, double dtime){
    /* Select different model for shear strain rate, controlled by flag_harden.
     * 0 : Voce Hardening; 1 : Dislocation Velocity Model;
     */
    //Update Schmidt here. But not realized yet, maybe pack the update into a function.
    Vector3d update_bv = burgers_vec;
    Vector3d update_nv = plane_norm;
    /* Pij = 0.5 * (update_bv / update_bv.norm() * update_nv.transpose() + update_nv * update_bv.transpose()/update_bv.norm()); */
    /* Rij = 0.5 * (update_bv / update_bv.norm() * update_nv.transpose() - update_nv * update_bv.transpose()/update_bv.norm()); */

    switch (flag_harden)
    {
        case 0:
            update_voce(gr_.gmode, gr_.lat_hard_mat, gr_.modes_num, dtime);
            break;
        case 1:
            update_disvel(gr_.gmode, gr_.lat_hard_mat, update_bv.norm(), gr_.modes_num, dtime);
            break;
        default:
            update_disvel(gr_.gmode, gr_.lat_hard_mat, update_bv.norm(), gr_.modes_num, dtime);
            break;
    }
}

void Slip::update_voce(Slip* gmode, MatrixXd lat_hard_mat, int modes_num, double dtime){
    /*
     * Update crss and acc_strain.
     */
    double Gamma = 0;
    for(int i = 0; i < modes_num; i++){
        Gamma += abs(gmode[i].acc_strain);
    }
    double tau_0 = harden_params[0], tau_1 = harden_params[1], h_0 = harden_params[2], h_1 = harden_params[3];
    double dtau_by_dGamma = h_1 + (abs(h_0/tau_1)*tau_1 - h_1) * exp(-Gamma*abs(h_0/tau_1)) + abs(h_0/tau_1)*h_1*Gamma*exp(-Gamma*abs(h_0/tau_1));
    for (int i = 0; i < modes_num; i++){
        crss += abs(gmode[i].strain_rate_slip) * dtime * lat_hard_mat(num,gmode[i].num) * dtau_by_dGamma;
    }
}

void Slip::update_disvel(Slip* slip_sys, MatrixXd lat_hard_mat, double bv, double nmode ,double dtime){
    /*
     * [velocity parameters] 
     *  1. MFP control coeffient, 2. reference frequency, 3. activation energy, 4. slip resistance, 5. energy exponent
     *  6. saturated speed, 7. drag coefficient
     * [hardening parameters] 
     *  8. forest hardening coefficient
     * [DD evolution parameters] 
     *  0. SSD_density, 9. nucleation coefficient, 10. nucleation threshold stress, 11. multiplication coefficient
     *  12. drag stress D, 13. reference strain rate, 14. c/g 
     *
     * update parameters:
     * 0: burgers, 1: mean_free_path, 2: disl_density_resist, 3: forest_stress
     */
    double c_mfp = harden_params[1], resistance_slip = harden_params[4], c_forest = harden_params[8], HP_stress = 0;
    double burgers, disl_density_for, disl_density_resist, joint_density, forest_stress, mfp;
    disl_density_for = disl_density_resist = joint_density = 0;
    for(int i = 0; i < nmode; i++){
        disl_density_for += slip_sys[i].disloc_density;
        disl_density_resist += slip_sys[i].disloc_density * (lat_hard_mat(num,slip_sys[i].num));
        if (slip_sys[i].num != num) {
            joint_density += lat_hard_mat(num,slip_sys[i].num) * sqrt(slip_sys[i].disloc_density) * sqrt(disloc_density);
        }
    }
    burgers = bv * 1e-10;
    forest_stress = c_forest * shear_modulus * burgers * sqrt(disl_density_resist + 0.707*joint_density);// + HP_stress
    crss = forest_stress + resistance_slip;
    mfp = c_mfp / sqrt(disloc_density);
    acc_strain += abs(strain_rate_slip) * dtime;
    update_params[0] = burgers, update_params[1] = mfp, update_params[2] = disl_density_resist, update_params[3] = forest_stress;
}

void Slip::update_ssd(Matrix3d strain_rate, double dtime){
    /*
     * [velocity parameters] 
     *  1. MFP control coeffient, 2. reference frequency, 3. activation energy, 4. slip resistance, 5. energy exponent
     *  6. saturated speed, 7. drag coefficient
     * [hardening parameters] 
     *  8. forest hardening coefficient
     * [DD evolution parameters] 
     *  0. SSD_density, 9. nucleation coefficient, 10. nucleation threshold stress, 11. multiplication coefficient
     *  12. drag stress D, 13. reference strain rate, 14. c/g 
     *
     * update parameters:
     * 0: burgers, 1: mean_free_path, 2: disl_density_resist, 3: forest_stress
     */
    if (flag_harden == 0){
        acc_strain += abs(strain_rate_slip) * dtime;
    }
    if (flag_harden == 1){ 
        double c_forest = harden_params[8], c_nuc = harden_params[9], tau_nuc = harden_params[10],\
               c_multi = harden_params[11], c_annih = 0.,\
               D = harden_params[12] * 1e6, ref_srate = harden_params[13], gg = c_forest/harden_params[14],\
               burgers = update_params[0], mfp = update_params[1], forest_stress = update_params[3]; 
        double equi_strain_rate = calc_equivalent_value(strain_rate);
        rho_sat = c_forest * burgers / gg * (1-k_boltzmann * temperature/D/pow(burgers,3) * log(abs(equi_strain_rate)/ref_srate));
        rho_sat = max(pow(1/rho_sat,2), 0.5*disloc_density);

        double term_nuc = c_nuc * max(abs(rss)-tau_nuc,0.) / (shear_modulus * burgers * burgers);
        double term_multi = c_multi / mfp; 
        c_annih = (term_multi + term_nuc) / rho_sat;
        disloc_density += (term_multi + term_nuc - c_annih * disloc_density) * abs(strain_rate_slip) * dtime;
        rho_mov = disloc_density;
        // if(disloc_density < rho_init) rho_init = SSD_density;
    }
}

void Slip::update_lhparams(Matrix3d strain_rate){
    if (flag_harden == 1){ 
        double ref_srate = 1e-3, exp_lh = -0.1;
        lh_coeff = pow(calc_equivalent_value(strain_rate)/ref_srate, exp_lh);
        if (lh_coeff > 2) lh_coeff = 2;
    }
    else{}
}

void Slip::cal_shear_modulus(Matrix6d elastic_modulus){
    Matrix3d slip_rotation;
    Vector3d trav_direc = burgers_vec.cross(plane_norm);
    slip_rotation << (burgers_vec/burgers_vec.norm()), plane_norm/plane_norm.norm(), trav_direc / trav_direc.norm();
    shear_modulus = rotate_6d_stiff_modu(elastic_modulus, slip_rotation.transpose())(3,3);
}

void Slip::print(){
    logger.info("Slip system: " + to_string(num));
    logger.info("Burgers vector: ");
    logger.info(burgers_vec.transpose());
    logger.info("Plane normal: ");
    logger.info(plane_norm.transpose());
    logger.info("Dislocation density: " + to_string(disloc_density));
    logger.info("Shear modulus: " + to_string(shear_modulus));
    logger.info("Strain rate: " + to_string(strain_rate_slip));
    logger.info("Accumulated strain: " + to_string(acc_strain));
    logger.info("Critical resolved shear stress: " + to_string(crss));
    logger.info("Harden parameters: ");
    string str = "";
    for (int i = 0; i < harden_params.size(); i++) str += to_string(harden_params[i]) + "  "; 
    logger.info(str);
    logger.info("LH coefficient: " + to_string(lh_coeff));
    logger.info("Flag harden: " + to_string(flag_harden));
}
