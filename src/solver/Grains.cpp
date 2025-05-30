#include "common/common.h"
#include "solver/Grains.h"
#include "mechanism/PMode.h"

using Eigen::MatrixXf;

grain::grain()
    : Cij6(), Cij6_SA_g(), Mij6_J_g(), Metilde_g(), Cij6_SA_g_old(), Mij6_J_g_old(),
      Metilde_g_old(), Mptilde_g(), Mpij6_g(), Mptilde_g_old(), Mpij6_g_old(),
      d0_g(), Fij_g(), Udot_g(), Dij_g(), Dije_g(), Dijp_g(),
      therm_expansion_g(), therm_strain_g(), Wij_g(), eps_g(), sig_g(),
      sig_g_old(), Dij_g_old(), 
      Dije_g_old(), Dijp_g_old(), d0_g_old(), therm_strain_g_old(),
      ell_axis_g(), ell_axisb_g(), ellip_ang_g(),
      Euler_M(),
      weight(0.0),
      gamma_delta_gmode(NULL),
      gamma_total(0.0),
      gamma_delta(0.0),
      grain_i(0),
      phase_id(0),
      modes_num(0),
      if_stress(0),
      size(0.0),
      child_frac(0.0),
      weight_ref(0.0),
      temperature(0.0),
      temp_old(0.0),
      twin_term_flag(false),
      mat(NULL),
      gmode(NULL),
      ell_crit_shape_g(CRIT_SHAPE),
      Iflat_g(false)
{
    // Cijkl, RSinv_C, RSinv_VP, RSinv_VP_old
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                for (int l = 0; l < 3; ++l) {
                    Cijkl[i][j][k][l] = 0.0;
                    RSinv_C[i][j][k][l] = 0.0;
                    RSinv_VP[i][j][k][l] = 0.0;
                    RSinv_VP_old[i][j][k][l] = 0.0;
                }
            }
        }
    }
}

grain::grain(const grain& other) :
    // 1. 基础类型成员赋值
    grain_i(other.grain_i),
    phase_id(other.phase_id),
    modes_num(other.modes_num),
    if_stress(other.if_stress),
    size(other.size),
    child_frac(other.child_frac),
    weight_ref(other.weight_ref),
    temperature(other.temperature),
    temp_old(other.temp_old),
    twin_term_flag(other.twin_term_flag),
    mat(other.mat), //shallow copy, assume mat is managed elsewhere
    gamma_total(other.gamma_total),
    gamma_delta(other.gamma_delta),
    weight(other.weight),
    ell_crit_shape_g(other.ell_crit_shape_g),
    Iflat_g(other.Iflat_g)
{
    // 2. Matrix/Vector 类成员
    Cij6 = other.Cij6;
    Cij6_SA_g = other.Cij6_SA_g;
    Mij6_J_g = other.Mij6_J_g;
    Metilde_g = other.Metilde_g;
    Cij6_SA_g_old = other.Cij6_SA_g_old;
    Mij6_J_g_old = other.Mij6_J_g_old;
    Metilde_g_old = other.Metilde_g_old;

    Mptilde_g = other.Mptilde_g;
    Mpij6_g = other.Mpij6_g;
    Mptilde_g_old = other.Mptilde_g_old;
    Mpij6_g_old = other.Mpij6_g_old;

    d0_g = other.d0_g;
    Fij_g = other.Fij_g;
    Udot_g = other.Udot_g;
    Dij_g = other.Dij_g;
    Dije_g = other.Dije_g;
    Dijp_g = other.Dijp_g;
    therm_expansion_g = other.therm_expansion_g;
    therm_strain_g = other.therm_strain_g;
    Wij_g = other.Wij_g;
    eps_g = other.eps_g;
    sig_g = other.sig_g;

    sig_g_old = other.sig_g_old;
    Dij_g_old = other.Dij_g_old;
    Dije_g_old = other.Dije_g_old;
    Dijp_g_old = other.Dijp_g_old;
    d0_g_old = other.d0_g_old;
    therm_strain_g_old = other.therm_strain_g_old;

    ell_axis_g = other.ell_axis_g;
    ell_axisb_g = other.ell_axisb_g;
    ellip_ang_g = other.ellip_ang_g;
    Euler_M = other.Euler_M;

    // Cijkl, RSinv_C, RSinv_VP, RSinv_VP_old
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                for (int l = 0; l < 3; ++l) {
                    Cijkl[i][j][k][l]       = other.Cijkl[i][j][k][l];
                    RSinv_C[i][j][k][l]     = other.RSinv_C[i][j][k][l];
                    RSinv_VP[i][j][k][l]    = other.RSinv_VP[i][j][k][l];
                    RSinv_VP_old[i][j][k][l]= other.RSinv_VP_old[i][j][k][l];
                }

    // 4. gamma_delta_gmode 数组
    if (other.gamma_delta_gmode && other.modes_num > 0) {
        gamma_delta_gmode = new double[other.modes_num];
        memcpy(gamma_delta_gmode, other.gamma_delta_gmode, other.modes_num * sizeof(double));
    } else {
        gamma_delta_gmode = NULL;
    }

    // 5. gmode 指针数组深拷贝
    if (other.gmode && other.modes_num > 0) {
        gmode = new PMode*[other.modes_num];
        for (int i = 0; i < other.modes_num; ++i) {
            gmode[i] = (other.gmode[i] ? new PMode(*other.gmode[i]) : NULL);
        }
    } else {
        gmode = NULL;
    }
    // 6. latent hardening matrix
    lat_hard_mat = other.lat_hard_mat;
}

grain::grain(grain&& other) noexcept :
    // 移动/复制基本类型
    grain_i(other.grain_i),
    phase_id(other.phase_id),
    modes_num(other.modes_num),
    if_stress(other.if_stress),
    size(other.size),
    child_frac(other.child_frac),
    weight_ref(other.weight_ref),
    temperature(other.temperature),
    temp_old(other.temp_old),
    twin_term_flag(other.twin_term_flag),
    mat(other.mat),  // 指针，移交所有权，但必须保证析构安全（看设计，若非owner，析构体注意）
    gamma_total(other.gamma_total),
    gamma_delta(other.gamma_delta),
    weight(other.weight),
    ell_crit_shape_g(other.ell_crit_shape_g),
    Iflat_g(other.Iflat_g)
{
    // 移动Eigen/自实现Matrix类型成员
    // 弹性矩阵相关成员
    Cij6             = std::move(other.Cij6);
    Cij6_SA_g        = std::move(other.Cij6_SA_g);
    Mij6_J_g         = std::move(other.Mij6_J_g);
    Metilde_g        = std::move(other.Metilde_g);
    Cij6_SA_g_old    = std::move(other.Cij6_SA_g_old);
    Mij6_J_g_old     = std::move(other.Mij6_J_g_old);
    Metilde_g_old    = std::move(other.Metilde_g_old);

    Mptilde_g        = std::move(other.Mptilde_g);
    Mpij6_g          = std::move(other.Mpij6_g);
    Mptilde_g_old    = std::move(other.Mptilde_g_old);
    Mpij6_g_old      = std::move(other.Mpij6_g_old);

    d0_g             = std::move(other.d0_g);
    Fij_g            = std::move(other.Fij_g);
    Udot_g           = std::move(other.Udot_g);
    Dij_g            = std::move(other.Dij_g);
    Dije_g           = std::move(other.Dije_g);
    Dijp_g           = std::move(other.Dijp_g);
    therm_expansion_g= std::move(other.therm_expansion_g);
    therm_strain_g   = std::move(other.therm_strain_g);
    Wij_g            = std::move(other.Wij_g);
    eps_g            = std::move(other.eps_g);
    sig_g            = std::move(other.sig_g);
    sig_g_old        = std::move(other.sig_g_old);
    Dij_g_old        = std::move(other.Dij_g_old);
    Dije_g_old       = std::move(other.Dije_g_old);
    Dijp_g_old       = std::move(other.Dijp_g_old);
    d0_g_old         = std::move(other.d0_g_old);
    therm_strain_g_old = std::move(other.therm_strain_g_old);

    // 椭球参数
    ell_axis_g       = std::move(other.ell_axis_g);
    ell_axisb_g      = std::move(other.ell_axisb_g);
    ellip_ang_g      = std::move(other.ellip_ang_g);
    Euler_M          = std::move(other.Euler_M);

    // 4维数组：手动循环赋值
    for(int i=0;i<3;++i)
        for(int j=0;j<3;++j)
            for(int k=0;k<3;++k)
                for(int l=0;l<3;++l) {
                    Cijkl[i][j][k][l]       = other.Cijkl[i][j][k][l];
                    RSinv_C[i][j][k][l]     = other.RSinv_C[i][j][k][l];
                    RSinv_VP[i][j][k][l]    = other.RSinv_VP[i][j][k][l];
                    RSinv_VP_old[i][j][k][l]= other.RSinv_VP_old[i][j][k][l];
                }
    // 注意：如果4维数组有资源型成员，则需要move，这里是double类型安全

    // 指针成员：简单move
    gamma_delta_gmode = other.gamma_delta_gmode;
    other.gamma_delta_gmode = nullptr;
    gmode = other.gmode;
    other.gmode = nullptr;

    // move所有vector（自动高效）
    lat_hard_mat = std::move(other.lat_hard_mat);

    // 重置modes_num，防止后续析构问题
    other.modes_num = 0;
    //（如mat仅为引用指针，不负责释放，建议不用重置；如有所有权，需析构前释放后置为nullptr）
}

grain& grain::operator=(const grain& other) {
    if (this != &other) {
        // 释放现有资源
        if (gamma_delta_gmode) {
            delete[] gamma_delta_gmode;
            gamma_delta_gmode = nullptr;
        }
        if (gmode) {
            for (int i = 0; i < modes_num; ++i) {
                delete gmode[i];
            }
            delete[] gmode;
            gmode = nullptr;
        }

        // 复制基本类型成员
        grain_i          = other.grain_i;
        phase_id         = other.phase_id;
        modes_num        = other.modes_num;
        if_stress        = other.if_stress;
        size             = other.size;
        child_frac       = other.child_frac;
        weight_ref       = other.weight_ref;
        temperature      = other.temperature;
        temp_old         = other.temp_old;
        twin_term_flag   = other.twin_term_flag;
        mat              = other.mat;
        gamma_total      = other.gamma_total;
        gamma_delta      = other.gamma_delta;
        weight           = other.weight;
        ell_crit_shape_g = other.ell_crit_shape_g;
        Iflat_g          = other.Iflat_g;

        // Matrix/Vector成员
        Cij6                = other.Cij6;
        Cij6_SA_g           = other.Cij6_SA_g;
        Mij6_J_g            = other.Mij6_J_g;
        Metilde_g           = other.Metilde_g;
        Cij6_SA_g_old       = other.Cij6_SA_g_old;
        Mij6_J_g_old        = other.Mij6_J_g_old;
        Metilde_g_old       = other.Metilde_g_old;

        Mptilde_g           = other.Mptilde_g;
        Mpij6_g             = other.Mpij6_g;
        Mptilde_g_old       = other.Mptilde_g_old;
        Mpij6_g_old         = other.Mpij6_g_old;

        d0_g                = other.d0_g;

        Fij_g               = other.Fij_g;
        Udot_g              = other.Udot_g;
        Dij_g               = other.Dij_g;
        Dije_g              = other.Dije_g;
        Dijp_g              = other.Dijp_g;
        therm_expansion_g   = other.therm_expansion_g;
        therm_strain_g      = other.therm_strain_g;
        Wij_g               = other.Wij_g;
        eps_g               = other.eps_g;
        sig_g               = other.sig_g;

        sig_g_old           = other.sig_g_old;
        Dij_g_old           = other.Dij_g_old;
        Dije_g_old          = other.Dije_g_old;
        Dijp_g_old          = other.Dijp_g_old;
        d0_g_old            = other.d0_g_old;
        therm_strain_g_old  = other.therm_strain_g_old;

        ell_axis_g          = other.ell_axis_g;
        ell_axisb_g         = other.ell_axisb_g;
        ellip_ang_g         = other.ellip_ang_g;
        Euler_M             = other.Euler_M;

        // 4维数组手动深拷贝
        for(int i=0;i<3;++i)
            for(int j=0;j<3;++j)
                for(int k=0;k<3;++k)
                    for(int l=0;l<3;++l) {
                        Cijkl[i][j][k][l]       = other.Cijkl[i][j][k][l];
                        RSinv_C[i][j][k][l]     = other.RSinv_C[i][j][k][l];
                        RSinv_VP[i][j][k][l]    = other.RSinv_VP[i][j][k][l];
                        RSinv_VP_old[i][j][k][l]= other.RSinv_VP_old[i][j][k][l];
                    }

        // 深度复制 gamma_delta_gmode
        gamma_delta_gmode = nullptr;
        if (other.gamma_delta_gmode && modes_num > 0) {
            gamma_delta_gmode = new double[modes_num];
            memcpy(gamma_delta_gmode, other.gamma_delta_gmode, modes_num * sizeof(double));
        }

        // 深度复制 gmode 数组
        gmode = nullptr;
        if (other.gmode && modes_num > 0) {
            gmode = new PMode*[modes_num];
            for (int i = 0; i < modes_num; ++i) {
                gmode[i] = (other.gmode[i] ? new PMode(*other.gmode[i]) : nullptr);
            }
        }

        // vector自动深拷贝
        lat_hard_mat = other.lat_hard_mat;
    }
    return *this;
}
grain& grain::operator=(grain&& other) noexcept {
    if (this != &other) {
        // 一、先释放当前对象资源，避免内存泄漏
        if (gamma_delta_gmode) {
            delete[] gamma_delta_gmode;
            gamma_delta_gmode = nullptr;
        }
        if (gmode) {
            for (int i = 0; i < modes_num; ++i) {
                delete gmode[i];
            }
            delete[] gmode;
            gmode = nullptr;
        }

        // 二、移动/复制基本类型成员
        grain_i          = other.grain_i;
        phase_id         = other.phase_id;
        modes_num        = other.modes_num;
        if_stress        = other.if_stress;
        size             = other.size;
        child_frac       = other.child_frac;
        weight_ref       = other.weight_ref;
        temperature      = other.temperature;
        temp_old         = other.temp_old;
        twin_term_flag   = other.twin_term_flag;
        mat              = other.mat;  // 指针所有权转移，确保析构安全
        gamma_total      = other.gamma_total;
        gamma_delta      = other.gamma_delta;
        weight           = other.weight;
        ell_crit_shape_g = other.ell_crit_shape_g;
        Iflat_g          = other.Iflat_g;

        // 三、移动Matrix/Vector成员
        Cij6                = std::move(other.Cij6);
        Cij6_SA_g           = std::move(other.Cij6_SA_g);
        Mij6_J_g            = std::move(other.Mij6_J_g);
        Metilde_g           = std::move(other.Metilde_g);
        Cij6_SA_g_old       = std::move(other.Cij6_SA_g_old);
        Mij6_J_g_old        = std::move(other.Mij6_J_g_old);
        Metilde_g_old       = std::move(other.Metilde_g_old);

        Mptilde_g           = std::move(other.Mptilde_g);
        Mpij6_g             = std::move(other.Mpij6_g);
        Mptilde_g_old       = std::move(other.Mptilde_g_old);
        Mpij6_g_old         = std::move(other.Mpij6_g_old);

        d0_g                = std::move(other.d0_g);

        Fij_g               = std::move(other.Fij_g);
        Udot_g              = std::move(other.Udot_g);
        Dij_g               = std::move(other.Dij_g);
        Dije_g              = std::move(other.Dije_g);
        Dijp_g              = std::move(other.Dijp_g);
        therm_expansion_g   = std::move(other.therm_expansion_g);
        therm_strain_g      = std::move(other.therm_strain_g);
        Wij_g               = std::move(other.Wij_g);
        eps_g               = std::move(other.eps_g);
        sig_g               = std::move(other.sig_g);

        sig_g_old           = std::move(other.sig_g_old);
        Dij_g_old           = std::move(other.Dij_g_old);
        Dije_g_old          = std::move(other.Dije_g_old);
        Dijp_g_old          = std::move(other.Dijp_g_old);
        d0_g_old            = std::move(other.d0_g_old);
        therm_strain_g_old  = std::move(other.therm_strain_g_old);

        ell_axis_g          = std::move(other.ell_axis_g);
        ell_axisb_g         = std::move(other.ell_axisb_g);
        ellip_ang_g         = std::move(other.ellip_ang_g);
        Euler_M             = std::move(other.Euler_M);

        // 四、内置数组循环赋值（这部分没法move，只能copy）
        for(int i=0;i<3;++i)
            for(int j=0;j<3;++j)
                for(int k=0;k<3;++k)
                    for(int l=0;l<3;++l) {
                        Cijkl[i][j][k][l]       = other.Cijkl[i][j][k][l];
                        RSinv_C[i][j][k][l]     = other.RSinv_C[i][j][k][l];
                        RSinv_VP[i][j][k][l]    = other.RSinv_VP[i][j][k][l];
                        RSinv_VP_old[i][j][k][l]= other.RSinv_VP_old[i][j][k][l];
                    }

        // 五、指针成员直接move
        gamma_delta_gmode = other.gamma_delta_gmode;
        other.gamma_delta_gmode = nullptr;
        gmode = other.gmode;
        other.gmode = nullptr;

        // vector成员move
        lat_hard_mat = std::move(other.lat_hard_mat);

        // 六、将源对象相关指针和计数重置
        other.modes_num = 0;
        other.mat = nullptr; // 如有所有权可置空
    }
    return *this;
}
grain::~grain() {
    if (gamma_delta_gmode) {
        delete[] gamma_delta_gmode;
        gamma_delta_gmode = nullptr;
    }
    
    if (gmode) {
        for (int i = 0; i < modes_num; ++i) {
            delete gmode[i];  // 删除每个PMode对象
        }
        delete[] gmode;       // 删除指针数组
        gmode = nullptr;
    }
    mat = nullptr;
}

void grain::initialization(int id, int phase_i, Vector4d euler, materialPhase* mat_pointer){
    grain_i = id;
    phase_id = phase_i;
    ini_euler_g(euler);
    mat = mat_pointer;
    Cij6 = mat->elasticConstants;
    voigt(Cij6, Cijkl);
    Vector6d temp_alpha = mat->thermalExpansionCoeffs.head<6>();
    therm_expansion_g = voigt(temp_alpha);
    size = mat->grainSize;
    ini_gmode_g(*mat);
    set_lat_hard_mat();
    for(int j = 0; j < modes_num; ++j){
        gmode[j]->update_status(*this,0.0);
    }
}

void grain::save_status_g(){
    // 1. 基本类型和简单成员
    temp_old             = temperature;
    sig_g_old            = sig_g;
    Mpij6_g_old          = Mpij6_g;
    Mptilde_g_old        = Mptilde_g;
    Dij_g_old            = Dij_g;        
    Dije_g_old           = Dije_g;
    Dijp_g_old           = Dijp_g;
    d0_g_old             = d0_g;
    therm_strain_g_old   = therm_strain_g;
    Cij6_SA_g_old        = Cij6_SA_g;
    Mij6_J_g_old         = Mij6_J_g;
    Metilde_g_old        = Metilde_g;

    // 3. 需要嵌套历史变量记忆的数组/Vector（如有，可以加for循环彻底覆盖）
    for(int i=0;i<3;++i)
        for(int j=0;j<3;++j)
            for(int k=0;k<3;++k)
                for(int l=0;l<3;++l) {
                    RSinv_C_old[i][j][k][l] = RSinv_C[i][j][k][l];
                    RSinv_VP_old[i][j][k][l]  = RSinv_VP[i][j][k][l];
                }
    // 5. gmode历史变量保存
    if (gmode && modes_num > 0) {
        for (int i = 0; i < modes_num; ++i){
            if(gmode[i]) gmode[i]->save_status();
        }
    }
}

void grain::restore_status_g() {
    temperature         = temp_old;
    sig_g               = sig_g_old;
    Mpij6_g             = Mpij6_g_old;
    Mptilde_g           = Mptilde_g_old;
    Dij_g               = Dij_g_old;
    Dije_g              = Dije_g_old;
    Dijp_g              = Dijp_g_old;
    d0_g                = d0_g_old;
    therm_strain_g      = therm_strain_g_old;
    Cij6_SA_g           = Cij6_SA_g_old;
    Mij6_J_g            = Mij6_J_g_old;
    Metilde_g           = Metilde_g_old;

    for(int i=0; i<3; ++i)
        for(int j=0; j<3; ++j)
            for(int k=0; k<3; ++k)
                for(int l=0; l<3; ++l) {
                    RSinv_C[i][j][k][l] = RSinv_C_old[i][j][k][l];
                    RSinv_VP[i][j][k][l]    = RSinv_VP_old[i][j][k][l];
                }

    if (gmode && modes_num > 0) {
        for (int i = 0; i < modes_num; ++i) {
            if(gmode[i]) {
                gmode[i]->restore_status();
                gmode[i]->update_temperature(temperature);
            }
        }
    }
}

Vector3d grain::get_euler_g(){return Euler_trans(Euler_M);}
Vector3d grain::get_ell_axis_g(){return ell_axis_g;}
Matrix3d grain::get_ell_axisb_g(){return ell_axisb_g;}
Matrix3d grain::get_stress_g(){return sig_g;}
void grain::set_stress_g(Matrix3d sig){sig_g = sig;}
Matrix3d grain::get_strain_g(){return eps_g;}
Matrix3d grain::get_Dije_g(){return Dije_g;}
Matrix3d grain::get_Dijp_g(){return Dijp_g;}
Matrix3d grain::get_Dij_g(){return Dij_g;}
Matrix3d grain::get_Udot_g(){return Udot_g;}
Matrix3d grain::get_Euler_M_g(){return Euler_M;}
Matrix6d grain::get_Cij6_g(){ return Cij6; }
double grain::get_weight_g(){return weight;}
void grain::set_weight_g(double w){weight = w;}

void grain::ini_euler_g(Vector4d vin)
{
    Vector3d euler = vin(seq(0,2));
    Euler_M = Euler_trans(euler);
    weight = vin(3);
    weight_ref = weight;
}

Vector3d grain::get_euler_g(int mode_num){
    if (gmode[mode_num]->type != mode_type::twin){
        logger.error("Not a twin, cannot output euler");
        return Vector3d::Zero();
    }
    Vector4d euler_vec = ((TwinG*)gmode[mode_num])->euler_twin;
    Vector3d v_out; v_out << euler_vec(0), euler_vec(1), euler_vec(2);
    return v_out;
}

double grain::get_weight_g(int mode_num){
    if (gmode[mode_num]->type != mode_type::twin){
        logger.error("Not a twin, cannot output weight");
        return 0;
    }
    Vector4d euler_vec = ((TwinG*)gmode[mode_num])->euler_twin;
    return euler_vec(3);
}

double grain::get_weight_g_eff(){return weight * (1-child_frac);}

int grain::ini_gmode_g(int n)
{
    modes_num = n;
    gmode = new PMode* [n];
    gamma_delta_gmode = new double[n];
    return 1;
}

int grain::ini_gmode_g(const materialPhase &mater){
    modes_num = mater.info_modes.size();
    gmode = new PMode* [modes_num];
    int i = 0;
    for (const ieMode &j_mode: mater.info_modes){
        switch (j_mode.type) {
            case 0:
                gmode[i] = new Slip(j_mode);
                break;
            case 1:
                gmode[i] = new TwinG(j_mode);
                break;
            case 2:
                gmode[i] = new PMode(j_mode);
                break;
            default:
                logger.error("Unknown mode type");
                return 0;
        }
        ++i;
    }
    logger.notice("Grain "+std::to_string(grain_i)+" has initiated "+std::to_string(modes_num)+" modes");
    /* for (int i = 0; i < modes_num; ++i){ */
    /*     gmode[i]->print(); */
    /* } */
    gamma_delta_gmode = new double[modes_num];
    return 1;
}

//Initial deformation mode based on the parent.
int grain::ini_gmode_g(grain &tp)
{
    modes_num = tp.modes_num;
    gmode = new PMode* [modes_num];
    bool new_mode = true;
    for (int i = 0; i < modes_num; ++i){
        switch (tp.gmode[i]->type) {
            case mode_type::slip:
                gmode[i] = new Slip((Slip*)tp.gmode[i], new_mode);
                break;
            case mode_type::twin:
                gmode[i] = new TwinG((TwinG*)tp.gmode[i], new_mode);
                break;
            case mode_type::undefined:
                gmode[i] = new PMode((PMode*)tp.gmode[i], new_mode);
                break;
            default:
                logger.error("Unknown mode type");
                return 0;
        }
    }
    gamma_delta_gmode = new double[modes_num];
    logger.notice("Grain "+std::to_string(grain_i)+" has initiated "+std::to_string(modes_num)+" modes");
    /* for (int i = 0; i < modes_num; ++i){ */
    /*     gmode[i]->print(); */
    /* } */
    return 1;
}

int grain::check_gmode_g(){return modes_num;}

int grain::check_sn_g(){
    for(int i = 0; i < modes_num; i++)
    {
        cout << "Mode " << i << ":\n";
        gmode[i]->check_sn_mode();
    }
    return 0;
}

int grain::check_hardening_g(){
    for(int i = 0; i < modes_num; i++)
    {
        logger.debug("Mode "+std::to_string(i)+":");
        gmode[i]->check_hardening_mode();
    }
    return 0;    
}

void grain::Eshelby_E(double ESIM[3][3][3][3], double ESCR[3][3][3][3], \
                      Vector3d axis_t, Matrix6d C66,Integralpoint6 aa6, Integralpoint6 aaww6, Integralpoint3 alpha)
{
    int npoints = Intn * Intn; 

    double aixabc = axis_t(0)*axis_t(1)*axis_t(2);
    //double ratio1 = axis_t(1)/axis_t(2);
    //double ratio2 = axis_t(0)/axis_t(2);


    Matrix6d T66; //Tijkl
    T66 = Matrix6d::Zero();

    Vector6d aa2, aaww2, a1;
    Vector6d a1_inv;
    Matrix3d a1m;

    double Ro3;
    double abcoro3;
    for(int ny = 0; ny < npoints; ny++)
    {
        aa2 = aa6.col(ny); //Take out the Alpha*Alpha
        a1 = Mult_voigt(C66,aa2);
        //C66(i,j,k,l)*aa2(j,l)
        //If solving an elastic inclusion invert the system
        //A(3,3) x X(3,3) = C(3,3)
        a1m = voigt(a1).inverse();
        a1_inv = voigt(a1m);

        Ro3 =0;
        for(int i = 0; i < 3; i++)
            Ro3 += pow(alpha(i,ny)*axis_t(i), 2);
        Ro3 = pow(Ro3, 1.5);
        abcoro3 = aixabc/Ro3;

        //T(M,N,I,J)=A(M)*A(J)*G(N,I)
        for(int i = 0; i < 6; i++)
            for(int j = 0; j < 6; j++)
                T66(i,j) += aaww6(i,ny)*a1_inv(j)*abcoro3;
    }
    double Tijkl[3][3][3][3];
    voigt(T66, Tijkl);

    //   COMPUTE SYMMETRIC (DISTORTION) ESHELBY TENSOR FROM EQ.B9.
    //       ESIM(N,M,K,L)=0.5*(T(M,J,N,I)+T(N,J,M,I))*C4(I,J,K,L)
    //   COMPUTE ANTI-SYMMETRIC (ROTATION) ESHELBY TENSOR FROM EQ.B9.
    //       ESCR(N,M,K,L)=0.5*(T(M,J,N,I)-T(N,J,M,I))*C4(I,J,K,L)
    double C4[3][3][3][3];
    voigt(C66, C4);

    double dumsim, dumscr;
    for(int l = 0; l < 3; l++)
        for(int k = 0; k < 3; k++)
            for(int m = 0; m < 3; m++)
                for(int n = 0; n < 3; n++)
                {
                    dumsim = 0;
                    dumscr = 0;
                    for(int j = 0; j < 3; j++)
                        for(int i = 0; i < 3; i++)
                        {
                            dumsim=dumsim+(Tijkl[m][j][n][i]+Tijkl[n][j][m][i])*C4[i][j][k][l];
                            dumscr=dumscr+(Tijkl[m][j][n][i]-Tijkl[n][j][m][i])*C4[i][j][k][l];
                        }
                    ESIM[n][m][k][l]=0.5*dumsim;
                    ESCR[n][m][k][l]=0.5*dumscr;
                }
}

void grain::Eshelby_VP(double ESIM[3][3][3][3],double ESCR[3][3][3][3],\
                      Vector3d axis_t, Matrix6d C66,Integralpoint6 aa6, Integralpoint6 aaww6, Integralpoint3 alpha, Integralpoint3 aww, Integralpoint1 ww)
{
    int npoints = Intn * Intn; 

    double aixabc = axis_t(0)*axis_t(1)*axis_t(2);
    //double ratio1 = axis_t(1)/axis_t(2);
    //double ratio2 = axis_t(0)/axis_t(2);

    Matrix6d T66; //Tijkl
    T66 = Matrix6d::Zero();

    Matrix3d P;
    P = Matrix3d::Zero();
    double PDIL = 0;

    Vector6d aa2, aaww2, a1;
    Vector6d a1_inv;
    Vector10d a1_10;
    Vector10d a1_10_inv;
    Matrix4d a1m;

    double Ro3;
    double abcoro3;
    for(int ny = 0; ny < npoints; ny++)
    {
        aa2 = aa6.col(ny); //Take out the Alpha*Alpha
        a1 = Mult_voigt(C66,aa2); //C66(i,j,k,l)*aa2(j,l)
        //refer to Equ[5-14b] in manual 7d)
        //solving a visco-plastic inclusion defines componets A1(7) TO A1(10).
        //solve the system given by A(4,4) X X(4,4) = C(4,4)
        a1_10(seq(0,5)) = a1;
        a1_10(6) = alpha(0,ny);
        a1_10(7) = alpha(1,ny);
        a1_10(8) = alpha(2,ny);
        a1_10(9) = 0;
        a1m = voigt(a1_10).inverse();
        a1_10_inv = voigt(a1m);

        Ro3 =0;
        for(int i = 0; i < 3; i++)
            Ro3 += pow(alpha(i,ny)*axis_t(i), 2);
        Ro3 = pow(Ro3, 1.5);
        abcoro3 = aixabc/Ro3;

        //T(M,N,I,J)=A(M)*A(J)*G(N,I)
        for(int i = 0; i < 6; i++)
            for(int j = 0; j < 6; j++)
                T66(i,j) += aaww6(i,ny)*a1_10_inv(j)*abcoro3;

        for(int j = 0; j < 3; j++)
            for(int i = 0; i < 3; i++)
                P(i,j) += aww(j,ny)*a1_10_inv(i+6)*abcoro3;
        PDIL += ww(ny)*a1_10_inv(9)*abcoro3;

    }
    double Tijkl[3][3][3][3];
    voigt(T66, Tijkl);

    //   COMPUTE SYMMETRIC (DISTORTION) ESHELBY TENSOR FROM EQ.B9.
    //       ESIM(N,M,K,L)=0.5*(T(M,J,N,I)+T(N,J,M,I))*C4(I,J,K,L)
    //   COMPUTE ANTI-SYMMETRIC (ROTATION) ESHELBY TENSOR FROM EQ.B9.
    //       ESCR(N,M,K,L)=0.5*(T(M,J,N,I)-T(N,J,M,I))*C4(I,J,K,L)
    double C4[3][3][3][3];
    voigt(C66, C4);

    double dumsim, dumscr;
    for(int l = 0; l < 3; l++)
        for(int k = 0; k < 3; k++)
            for(int m = 0; m < 3; m++)
                for(int n = 0; n < 3; n++)
                {
                    dumsim = 0;
                    dumscr = 0;
                    for(int j = 0; j < 3; j++)
                        for(int i = 0; i < 3; i++)
                        {
                            dumsim=dumsim+(Tijkl[m][j][n][i]+Tijkl[n][j][m][i])*C4[i][j][k][l];
                            dumscr=dumscr+(Tijkl[m][j][n][i]-Tijkl[n][j][m][i])*C4[i][j][k][l];
                        }
                    ESIM[n][m][k][l]=0.5*dumsim;
                    ESCR[n][m][k][l]=0.5*dumscr;
                }
}

//elastic consistent
void grain::Update_Cij6_SA_g(Matrix6d Min){Cij6_SA_g = Min;}
void grain::Update_Mij6_J_g(Matrix6d Min){Mij6_J_g = Min;}
void grain::Update_Metilde_g(Matrix6d Min){Metilde_g = Min;}
Matrix6d grain::get_Metilde_g(){return Metilde_g;}
void grain::Update_RSinv_C_g(double A[3][3][3][3])
{
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            for(int m = 0; m < 3; m++)
                for(int n = 0; n < 3; n++)
                    RSinv_C[i][j][m][n] = A[i][j][m][n];
}
Matrix6d grain::get_Mij6_J_g(){return Mij6_J_g;}

//visco-plastic consistent
void grain::Update_Mptilde_g(Matrix5d Min){Mptilde_g = Min;}
Matrix5d grain::get_Mpij6_g(){return Mpij6_g;}
Vector5d grain::get_d0_g(){return d0_g;}
void grain::Update_RSinv_VP_g(double A[3][3][3][3])
{
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            for(int m = 0; m < 3; m++)
                for(int n = 0; n < 3; n++)
                    RSinv_VP[i][j][m][n] = A[i][j][m][n];
}

void grain::Update_Mpij6_g(int linear_mode)
{
    Matrix5d M_tangent = 1e-10 * Matrix5d::Identity() + cal_Fgrad(sig_g) ;
    Matrix5d M_secant = 1e-10 * Matrix5d::Identity() + cal_M_secant(sig_g);
    Vector5d d0_g_tg = Chg_basis5(Dijp_g) - M_tangent * Chg_basis5(sig_g);
    Vector5d d0_g_affine = (M_secant - M_tangent) * Chg_basis5(sig_g);
    /* Affine Linearization: */
    if (linear_mode == 1){
        Mpij6_g = M_tangent;
        d0_g = d0_g_affine;
    }
    /* Tangent Linearization: */
    if (linear_mode == 2){
        Mpij6_g = M_tangent;
        d0_g = d0_g_tg;
    }
    /* Secant Linearization: */
    if (linear_mode == 3){
        Mpij6_g = M_secant;
        d0_g = Chg_basis5(Dijp_g) - Mpij6_g * Chg_basis5(sig_g);
    }
    else{
        Mpij6_g = M_secant;
        d0_g = Chg_basis5(Dijp_g) - Mpij6_g * Chg_basis5(sig_g);
    }
    if ((isnan(Mpij6_g.determinant())) || (isnan(d0_g.norm()))){
        logger.warn("Mpij6_g or d0_g is nan, Grain number" + to_string(grain_i));
        Mpij6_g = Mpij6_g_old; d0_g = d0_g_old;
    }
}

void grain::Update_shape_g()
{
    //-1 F.transpose()*F
    //BX = Matrix3d::Zero();
    //for(int i = 0; i < 3; i++)
    //    for(int j = 0; j < 3; j++)
    //        BX(i,j) = Fij_g.row(i)*Fij_g.col(j);
    Matrix3d BX;
    Matrix3d FT = Fij_g.transpose();
    BX =  Fij_g*FT;
    //-1 F.transpose()*F

    //-2 solve the eigen vector of BX
    //and sort the value from largest to smallest
    EigenSolver<MatrixXd> es(BX);
    Matrix3d BX_vectors;
    Vector3d BX_value;
    BX_value = (es.eigenvalues()).real();
    BX_vectors = (es.eigenvectors()).real();
    Eigsrt(BX_vectors, BX_value);
    //-2 solve the eigen vector of BX
    //and sort the value from largest to smallest

    Matrix3d B = BX_vectors;
    Vector3d W = BX_value;
    //-3 
    //redefine Axis(1) (the second) to be the largest  
    //to improve the accuracy in calculation of Eshelby tensor
    //IF DET(B)<0 MEANS THAT THE SYSTEM IS LEFT HANDED. IT IS MADE RIGHT
    //HANDED BY EXCHANGING 1 AND 2.
    double Sign = -1;
    double temp;
    if(B.determinant() <= 0) Sign = 1;
    for(int i = 0; i < 3; i++)
    {
        temp = B(i,0);
        B(i,0) = B(i,1);
        B(i,1) = temp * Sign;
    }
    temp = W(0); W(0) = W(1); W(1) = temp;
    //-3 

    //-4 update the stretching of ellipsoid
    double Ratmax=sqrt(W(1)/W(2));
    double Ratmin=sqrt(W(0)/W(2));

    ell_axisb_g = B;
    if(!Iflat_g) //if Iflat_g = 0
        for(int i = 0; i < 3; i++)
            ell_axis_g(i) = sqrt(W(i));
    //if Iflat_g = 1, the axis of ellipsoid keeps unchange
    //-4 update the stretching of ellipsoid

    //-5 update the ellipsoid orientation
    Matrix3d BT = B.transpose();
    ellip_ang_g = Euler_trans(B);
    //-5

    //-6 Update the Iflat_g according to the Max axes ratio of ellipsoid
    if((!Iflat_g)&&(Ratmax >= ell_crit_shape_g))
        {
            Iflat_g = 1;
        }
        //-6

        //-7 Iflat_g = 1; recaculates the Fij_g in grain
    else if((Iflat_g)&&(Ratmax >= ell_crit_shape_g))
    {
    W(1) = W(1)/4;
    if(Ratmin >= 0.5 * ell_crit_shape_g)
        W(0) = W(0)/4;
    for(int i = 0; i < 3; i++)
        ell_axis_g(i) = sqrt(W(i));

    Matrix3d Fijx;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            Fijx(i,j) = Iij(i,j) * ell_axis_g(i);

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
        {
            Fij_g(i,j) = 0;
            for(int k = 0; k < 3; k++)
                for(int l = 0; l < 3; l++)
                    Fij_g(i,j) = Fij_g(i,j) + B(i,k)*B(j,l)*Fijx(k,l);
        }
}
}

void grain::Update_Fij_g(double Tincr)
{
    Matrix3d Fnew;
    Fnew = Matrix3d::Zero();
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            for(int k = 0; k < 3; k++)
                Fnew(i,j) += (Tincr*Udot_g(i,k)+Iij(i,k))*Fij_g(k,j);
    Fij_g = Fnew;
}

Matrix3d grain::cal_Dijp(Matrix3d Min) //更改，使得可以输出电流张量的转换结果
{
    //transform into the deviatoric tensor
    Matrix3d X = devia(Min);
    Matrix3d E = Euler_M;
    Matrix3d ET = Euler_M.transpose();
    X = E * X * ET;
    
    Matrix3d Dijp = Matrix3d::Zero();
    for(int i = 0; i < modes_num; i++)
    {
        Dijp += gmode[i]->cal_dijpmode(X);
    }
    return ET*Dijp*E;
}

Matrix3d grain::cal_rotslip()
{
    Matrix3d E = Euler_M;
    Matrix3d ET = E.transpose();

    Matrix3d Wij = Matrix3d::Zero();
    for(int i = 0; i < modes_num; i++)
    {
        Wij += gmode[i]->cal_rot_mode();
    }
    return ET*Wij*E;
}
//test here

Matrix5d grain::cal_Fgrad(Matrix3d Min)
{
    Matrix3d X = devia(Min);
    Matrix3d E = Euler_M;
    Matrix3d ET = E.transpose();
    X = E * X * ET;
    Matrix6d Fgrad = Matrix6d::Zero();
    for(int i = 0; i < modes_num; i++)
        Fgrad += gmode[i]->get_Fgradm(X);
    return Chg_basis5(rotate_C66(Fgrad, ET));
}

Matrix5d grain::cal_M_secant(Matrix3d Min)
{
    Matrix3d X = devia(Min);
    Matrix3d E = Euler_M;
    Matrix3d ET = E.transpose();
    X = E * X * ET;
    Matrix6d M_sec = Matrix6d::Zero();
    for(int i = 0; i < modes_num; i++)
        M_sec += gmode[i]->get_M_secant(X);
    return Chg_basis5(rotate_C66(M_sec, ET));
}

double grain::cal_RSSxmax(Matrix3d Min)
{
    Matrix3d X = devia(Min);
    Matrix3d E = Euler_M;
    Matrix3d ET = Euler_M.transpose();
    X = E * X * ET;
    double RSSxmax = 0;
    for(int i = 0; i < modes_num; i++)
    {
        double temp = gmode[i]->cal_relative_rss(X);
        if(RSSxmax < temp) RSSxmax = temp;
    }
    return RSSxmax;
}

double grain::cal_RSSxlim(Matrix3d D)
{   
    double nrsxmin = 0;
    for(int i = 0; i < modes_num; i++)
        if(nrsxmin > gmode[i]->get_nrsx())
            nrsxmin = gmode[i]->get_nrsx();

    double lim = 2* pow(D.norm()/gmode[0]->get_gamma0(),1/nrsxmin);
    if(lim < 2) lim = 2;
    return  lim;           
}

void grain::grain_stress_evp(double Tincr, Matrix3d Wij_m, Matrix3d Dij_m,\
                         Matrix3d Dije_AV, Matrix3d Dijp_AV, Matrix3d Sig_m, Matrix3d Sig_m_old, Matrix3d therm_expansion_ave)
{
    if_stress = 1;
    Matrix3d X = sig_g; 
    //according to Equ[5-30] in manual, calculate the wij~ and the Jaumann rate
    Matrix3d wg = Wij_m + mult_4th(RSinv_C,Dije_g-Dije_AV) + mult_4th(RSinv_VP,Dijp_g-Dijp_AV);
    Matrix3d stress_ave_jau = (Sig_m - Sig_m_old)/Tincr - (Wij_m * Sig_m - Sig_m * Wij_m);
    Matrix3d stress_g_jau_diff = -sig_g_old/Tincr - wg * X + X * wg; // grain stress jaumannn rate = stress_grain/dt + this term
    Matrix3d stress_jau_diff = stress_g_jau_diff - stress_ave_jau; // Difference in jaumann rate is stress_grain/dt + this term

    //solve the interaction equation of grain
    Vector6d Me_tilde_term = Metilde_g * Chg_basis6(stress_jau_diff);
    Vector6d Me_g_term = Mij6_J_g * Chg_basis6(stress_g_jau_diff);
    Vector6d Mp_tilde_term = B5to6(Mptilde_g * Chg_basis5(Sig_m));
    Vector6d F_ind_terms = Chg_basis6(Dij_m) - Me_tilde_term - Me_g_term + Mp_tilde_term;
    //thermal expansion part of interaction
    double temperature_diff = temperature - temperature_ref;
    Matrix3d therm_expansion_g_sys = Euler_M.transpose() * therm_expansion_g * Euler_M;
    Matrix3d thermal_strain_rate = (therm_expansion_g_sys * temperature_diff - therm_strain_g_old) / Tincr;
    F_ind_terms -= Chg_basis6(thermal_strain_rate);

    //Newton-Rapthon iteration
    Vector6d Xv = Chg_basis6(X), Xold = Xv, F = Vector6d::Zero();
    /* logger.debug("initial Xv:"); logger.debug(Xv); */
    Vector5d dijpgv;
    Matrix6d Fgrad, Mtemp = (Metilde_g + Mij6_J_g)/Tincr;
    double Errm = 1e-3, F_err = 1e-3, err_iter = Errm*10, coeff = 1;
    int NR_max_iter = 7, DH_max_iter = 25;
    for(int it = 0; it < NR_max_iter; it ++ )
    {
        Vector6d Metilde_g_xterm = Metilde_g * Xv / Tincr;
        Vector6d M_J_g_xterm = Mij6_J_g * Xv / Tincr;
        Vector6d Mp_tilde_g_xterm = B5to6(Mptilde_g * Chg_basis5(Chg_basis(Xv)));
        dijpgv = Chg_basis5(cal_Dijp(Chg_basis(Xv)));
        Vector6d dp_g_xterm = B5to6(dijpgv);
        F = -F_ind_terms +  Metilde_g_xterm + M_J_g_xterm + Mp_tilde_g_xterm + dp_g_xterm;
        Fgrad = -Bbasisadd(Mtemp,Mptilde_g+cal_Fgrad(Chg_basis(Xv)));
        Xold = Xv;
        Xv = Xold + Fgrad.inverse() * F;
        err_iter = Errorcal(Xv, Xold);
        if(err_iter < Errm) break;
    }
    if (isnan(err_iter)){
        logger.warn("Grain stress iteration failed (nan)! Grain " + to_string(grain_i));
        Xv = Chg_basis6(sig_g); F = Vector6d::Ones();
        if_stress = 0;
    }
    /* logger.debug("Xv after NR iteration: "); logger.debug(Xv); */
    /* logger.debug("F norm after NR iteration: " + to_string(F.norm())); */
    if(F.norm() >= F_err){
        /* logger.warn("Entering downhill ..."); */
        double F_norm = F.norm();
        for (int it = 0; F_norm >= F_err; it++) {
            Vector6d Metilde_g_xterm = Metilde_g * Xv / Tincr;
            Vector6d M_J_g_xterm = Mij6_J_g * Xv / Tincr;
            Vector6d Mp_tilde_g_xterm = B5to6(Mptilde_g * Chg_basis5(Chg_basis(Xv)));
            dijpgv = Chg_basis5(cal_Dijp(Chg_basis(Xv)));
            Vector6d dp_g_xterm = B5to6(dijpgv);
            F = -F_ind_terms +  Metilde_g_xterm + M_J_g_xterm + Mp_tilde_g_xterm + dp_g_xterm;

            if (F.norm() < F_norm) {
                F_norm = F.norm();
                coeff = min(1., coeff * 2);
                Xold = Xv;
                Fgrad = -Bbasisadd(Mtemp, Mptilde_g + cal_Fgrad(Chg_basis(Xv)));
                Xv = Xold + coeff * Fgrad.inverse() * F;
            }
            else{
                coeff *= 0.5;
                Xv = Xold + (Xv - Xold) * 0.5;
            }
            /* logger.debug("Grain stress (downhill) iteration: " + to_string(it)); */
            /* logger.debug(Xv); */
            /* if(grain_i == 0) logger.debug("Grain stress iteration: " + to_string(it) + ", F_norm: " + to_string(F_norm) + ", coeff: " + to_string(coeff)); */
            if ((it == DH_max_iter) || (coeff < 1e-4)) {
                logger.warn("Grain stress iteration failed ! Grain " + to_string(grain_i) + ", F_norm: " + to_string(F_norm));
                /* Xv = Chg_basis6(X) + Chg_basis6(Sig_m) - Chg_basis6(Sig_m_old); */
                Xv = Chg_basis6(Sig_m);
                if_stress = 0;
                break;
            }
        }
    }
    /**/
    sig_g = Chg_basis(Xv);
    Vector6d dijegv =  Mij6_J_g *(Xv/Tincr + Chg_basis6(stress_g_jau_diff)); 
    dijpgv = Chg_basis5(cal_Dijp(sig_g));
    Dije_g = Chg_basis(dijegv); Dijp_g = Chg_basis(dijpgv);
    therm_strain_g = therm_expansion_g_sys * temperature_diff;
    Dij_g = Dije_g + Dijp_g + thermal_strain_rate;
    Update_Mpij6_g(3);
}

void grain::update_strain(double Tincr)
{
    eps_g += Dij_g * Tincr;
}

Matrix3d grain::get_Wij_g()
{
    return Wij_g;
}

void grain::update_orientation(double Tincr, Matrix3d Wij_m, Matrix3d Dije_AV, Matrix3d Dijp_AV)
{
    Wij_g = Wij_m + mult_4th(RSinv_C,Dije_g-Dije_AV) + mult_4th(RSinv_VP,Dijp_g-Dijp_AV);

    Udot_g = Wij_g + Dij_g; //update the velocity gradient in grains

    Matrix3d Rot = (Wij_g - cal_rotslip())*Tincr;
    Matrix3d Euler_M_new = Euler_M*Rodrigues(Rot).transpose();
    if (Errorcal(Euler_M, Euler_M_new) < 0.33) Euler_M = Euler_M_new;
    /* Euler_M = Euler_M_new; */
}

void grain::update_modes(double Tincr)
{
    for(int i = 0; i < modes_num; i++)	gmode[i]->update_ssd(Dij_g, Tincr);
    for(int i = 0; i < modes_num; i++)	gmode[i]->update_ssd_coplanar_reaction(modes_num, gmode, Tincr);
    child_frac = 0.;
    for (int i = 0; i < modes_num; i++) {
        if (gmode[i]->type != mode_type::twin) continue;
        double probe_child_frac = ((TwinG*)gmode[i])->child_frac;
        child_frac += probe_child_frac;
    }
    if (child_frac > global_polycrys.twin_threshold) {
        twin_term_flag = true;
        for (int i = 0; i < modes_num; i++) {
            if (gmode[i]->type != mode_type::twin) continue;
            ((TwinG*)gmode[i])->set_status(twin_status::saturated);
        }
    }
    else twin_term_flag = false;
    for(int i = 0; i < modes_num; i++)	gmode[i]->update_status(*this, Tincr);
    gamma_total += gamma_delta;
}

void grain::set_lat_hard_mat(){
    vector<vector<double>> latent_matrix;
    latent_matrix.resize(modes_num);
    for (int i = 0; i < modes_num; i++) latent_matrix[i].resize(modes_num);
    for (int i = 0; i < modes_num; i++){
        for (int j = 0; j < modes_num; j++){
            if (i == j) latent_matrix[i][j] = 1;
            else{
                int mode = 0;
                mode_type type_i = gmode[i]->type;
                mode_type type_j = gmode[j]->type;
                if (type_i == mode_type::slip && type_j == mode_type::slip) {
                    mode = get_interaction_mode(gmode[i]->burgers_vec, gmode[i]->plane_norm, gmode[j]->burgers_vec, gmode[j]->plane_norm);
                }
                else if (type_i == mode_type::slip && type_j == mode_type::twin) mode = 5;
                else if (type_i == mode_type::twin && type_j == mode_type::slip) mode = 0;
                else if (type_i == mode_type::twin && type_j == mode_type::twin) mode = 1;
                else mode = 0;
                latent_matrix[i][j] = gmode[i]->latent_params[mode];
            }
        }
    }
    lat_hard_mat = latent_matrix;
}

void grain::print_latent_matrix(){
    string mat_row = "";
    for (int i = 0; i < modes_num; i++){
        for (int j = 0; j < modes_num; j++){
            mat_row += to_string(lat_hard_mat[i][j]) + " ";
        }
        logger.debug(mat_row);
        mat_row = "";
    }
}

void grain::therm_expansion_config(Vector6d therm){
    /* order: 11 22 33 23 13 12 */
    therm_expansion_g = voigt(therm);
}

Matrix3d grain::get_therm_expansion(){
    return Euler_M.transpose() * therm_expansion_g * Euler_M;
}

// A default template of the temperature evolution
void grain::update_temperature(double time_incre)
{
    //temperature = temperature + (Tincr/(rho_material*Cp_material))*(Dijp_g.cwiseProduct(sig_g).sum()+pow(J_intensity_pulse(Tincr,duty_ratio_J,Amplitude_J,Frequency),2)/sigma_e_mat);
    //不用赋予初值，在processes里面有// temperature of the atmosphere is a global variable, which can be directly used here
    temperature = 293.0; //test mode
    /* temperature += slope_profile_incr(Tincr, -10); //test mode
    logger.debug("Temperature of grain " + to_string(grain_i) + " is " + to_string(temperature) + " K."); */

//1.29 test mode
    // double electric_intensity = J_intensity_pulse(time_incre,duty_ratio_J,Amplitude_J,Frequency);

    // double scale = time_incre / (rho_material * Cp_material);
    // double plas_dissipation_term = Dijp_g.cwiseProduct(sig_g).sum();
    // double electricity_induced_term = pow(electric_intensity,2) / sigma_e_mat;
    // double temperature_incre = scale * (plas_dissipation_term + electricity_induced_term);
    // temperature = temperature + temperature_incre;

//1.29 test mode

    // temperature of the atmosphere is a global variable, which can be directly used here

}

// void grain::update_jslip(){
//     // J_tensor(2,2) = J_intensity_pulse(time_acc, duty_ratio_J, Amplitude_J, Frequency);
//     // Matrix3d J_grain = Euler_M * J_tensor * Euler_M.transpose(); tensorial convention

//     Vector3d J_real;
//     J_real(0) = 0, J_real(1) = 0,J_real(2) = J_intensity_pulse(time_acc, duty_ratio_J, Amplitude_J, Frequency);
//     J_real = Euler_M*J_real;
//     for(int i=0;i< modes_num; i++){
//         if (gmode[i]->type != mode_type::slip)continue;
//         double J_eq = J_real.transpose()* gmode[i]-> plane_norm;
//         gmode[i]-> J_slipsystem = J_eq;
//     }
// }


//张量convention的电流密度
void grain::update_jslip(){
    J_tensor(2,2) = J_intensity_pulse(time_acc, duty_ratio_J, Amplitude_J, Frequency);
    Matrix3d J_grain = Euler_M * J_tensor * Euler_M.transpose(); //tensorial convention

    // Vector3d J_real;
    // J_real(0) = 0, J_real(1) = 0,J_real(2) = J_intensity_pulse(time_acc, duty_ratio_J, Amplitude_J, Frequency);
    // J_real = Euler_M*J_real;


    for(int i=0;i< modes_num; i++){
        if (gmode[i]->type != mode_type::slip)continue;
        double J_eq = J_tensor.cwiseProduct(gmode[i]->Pij).sum();
        gmode[i]-> J_slipsystem = J_eq;
    }
}
