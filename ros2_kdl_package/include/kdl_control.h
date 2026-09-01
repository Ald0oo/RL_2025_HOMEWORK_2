#ifndef KDLControl
#define KDLControl

#include "Eigen/Dense"
#include "kdl_robot.h"
#include "utils.h"

class KDLController
{

public:

    KDLController(KDLRobot &_robot);

    Eigen::VectorXd idCntr(KDL::JntArray &_qd,
                           KDL::JntArray &_dqd,
                           KDL::JntArray &_ddqd,
                           double _Kp,
                           double _Kd);

    Eigen::VectorXd idCntr(KDL::Frame &_desPos,
                           KDL::Twist &_desVel,
                           KDL::Twist &_desAcc,
                           double _Kpp,
                           double _Kpo,
                           double _Kdp,
                           double _Kdo);

    Eigen::VectorXd velocity_ctrl_null(const Eigen::VectorXd &cart_vel_cmd, double lambda);

    Eigen::VectorXd vision_ctrl(
        const Eigen::Vector3d& s,
         const Eigen::VectorXd& q, 
         double dt);
   
private:

    KDLRobot* robot_;

    Eigen::VectorXd compute_null_space_q0(double lambda);

};

#endif
