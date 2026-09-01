#include "kdl_control.h"

KDLController::KDLController(KDLRobot &_robot)
{
    robot_ = &_robot;
}

Eigen::VectorXd KDLController::idCntr(KDL::JntArray &_qd,
                                      KDL::JntArray &_dqd,
                                      KDL::JntArray &_ddqd,
                                      double _Kp, double _Kd)
{
    // read current state
    Eigen::VectorXd q = robot_->getJntValues();
    Eigen::VectorXd dq = robot_->getJntVelocities();

    // calculate errors
    Eigen::VectorXd e = _qd.data - q;
    Eigen::VectorXd de = _dqd.data - dq;

    Eigen::VectorXd ddqd = _ddqd.data;
    return robot_->getJsim() * (ddqd + _Kd*de + _Kp*e)
            + robot_->getCoriolis() + robot_->getGravity() /*friction compensation?*/;
}

Eigen::VectorXd KDLController::idCntr(KDL::Frame &_desPos,
                                      KDL::Twist &_desVel,
                                      KDL::Twist &_desAcc,
                                      double _Kpp, double _Kpo,
                                      double _Kdp, double _Kdo)
{

}

Eigen::VectorXd KDLController::compute_null_space_q0(double lambda)
{
    unsigned int n = robot_->getNrJnts();
    Eigen::VectorXd q = robot_->getJntValues();
    Eigen::MatrixXd limits = robot_->getJntLimits();
   
    Eigen::VectorXd q_min = limits.col(0); 
    Eigen::VectorXd q_max = limits.col(1);
    Eigen::VectorXd q0_dot(n);
    q0_dot.setZero();

    for (unsigned int i = 0; i < n; ++i)
    {
        double q_i = q(i);
        double q_max_i = q_max(i);
        double q_min_i = q_min(i);

        double C_i = (1.0 / lambda) * std::pow(q_max_i - q_min_i, 2);
        double num_grad = C_i * (2 * q_i - q_max_i - q_min_i);
        double den_grad_term1 = (q_max_i - q_i);
        double den_grad_term2 = (q_i - q_min_i);
        double den_grad = std::pow(den_grad_term1 * den_grad_term2 + 1e-9, 2);

        q0_dot(i) = (num_grad / den_grad);
    }
    return q0_dot;
    return Eigen::VectorXd::Zero(robot_->getNrJnts());
}

Eigen::VectorXd KDLController::velocity_ctrl_null(const Eigen::VectorXd &cart_vel_cmd, double lambda)
{
    unsigned int n = robot_->getNrJnts();
    KDL::Jacobian J_kdl = robot_->getEEJacobian();
    Eigen::MatrixXd J = J_kdl.data;
    Eigen::MatrixXd J_pinv = pseudoinverse(J);

    Eigen::VectorXd q0_dot = this->compute_null_space_q0(lambda);

    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(n, n);
    Eigen::MatrixXd N = I - J_pinv * J;

    Eigen::VectorXd q_dot_task = J_pinv * cart_vel_cmd;
    Eigen::VectorXd q_dot_null = N * q0_dot;

    Eigen::VectorXd q_dot = q_dot_task + q_dot_null;

    return q_dot;
}

Eigen::VectorXd KDLController::vision_ctrl(const Eigen::Vector3d& s, const Eigen::VectorXd& q, double dt)
{
    (void)dt;
    // Dichiara l'offset della telecamera (modifica i valori 0.0 con quelli reali se li hai)
    Eigen::Vector3d cPo(0.0, 0.0, 0.0);
    
    Eigen::Matrix3d R;
    R <<    -1,  0,  0,
             0, -1,  0,
             0,  0,  1;

    Eigen::Matrix3d skew;
    skew <<        0, -s(2),  s(1),
                s(2),     0, -s(0),
               -s(1),  s(0),     0;

     
    Eigen::Vector3d camera_pos = R * cPo;
    robot_->update(toStdVector(q), std::vector<double>(robot_->getNrJnts(), 0.0));
    KDL::Frame ee_frame = robot_->getEEFrame();
    Eigen::Matrix3d R_world_to_ee = toEigen(ee_frame.M);
    Eigen::Vector3d p_ee(ee_frame.p.data);
    double dist = camera_pos.norm();
    if (dist < 1e-6)
        dist = 1e-6;

   
    Eigen::Vector3d s_curr = camera_pos / dist;   
    Eigen::Vector3d s_d(0.0, 0.0, 1.0);            
    Eigen::Matrix3d I = Eigen::Matrix3d::Identity();
    Eigen::MatrixXd L(3,6);
    L.block<3,3>(0,0) = -(I - s * s.transpose()) / dist;
    L.block<3,3>(0,3) = skew;
    Eigen::MatrixXd J_ee = robot_->getEEJacobian().data;
    int n_jnts = J_ee.cols();
    Eigen::MatrixXd Jc = J_ee;
    Eigen::MatrixXd LJ = L * Jc;
    Eigen::MatrixXd LJ_pinv = pseudoinverse(LJ);
    Eigen::MatrixXd N = Eigen::MatrixXd::Identity(n_jnts, n_jnts) - LJ_pinv * LJ;
    Eigen::VectorXd q0_dot = Eigen::VectorXd::Zero(n_jnts);
    Eigen::Vector3d s_error = s_d - s;
    double K_v = 1.0;
    Eigen::VectorXd q_dot_task = K_v * (LJ_pinv * s_error);
    Eigen::VectorXd q_dot_null = N * q0_dot;
    Eigen::VectorXd q_dot = q_dot_task + q_dot_null;

    return q_dot;
}
