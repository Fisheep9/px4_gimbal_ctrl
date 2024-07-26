
/* This is a example of controlling gimbal using mavros mount control */
#include <ros/ros.h>
#include <mavros_msgs/MountControl.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/SetMode.h>
#include <geometry_msgs/Quaternion.h>

#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <cmath>

mavros_msgs::State current_state;

bool last_pos_updated = false;


double position{0.0};
double bias{0.0};
double range{45.0};

double mag{0.0};
double freq{0.0};
double offset{0.0};

double loop_rate{50.0};
double rate{50.0};

std::string type = "fixed";



void state_cb(const mavros_msgs::State::ConstPtr& msg) {
	    current_state = *msg;
}

void gimbal_pos_cb(const geometry_msgs::Quaternion::ConstPtr& msg) {
		if (!last_pos_updated) {
			tf2::Quaternion tf_quat;
    		tf2::convert(*msg, tf_quat);
    		double roll, pitch, yaw;
    		tf2::Matrix3x3(tf_quat).getRPY(roll, pitch, yaw);
			// update 
			double last_position = std::ceil(roll*180/M_PI);
			position = last_position;
			last_pos_updated = true;
		}
}

void position_planner(double &target, double &current_pos) {

	if (target != NAN && last_pos_updated) {
		if (current_pos > target) {
		current_pos -= rate / loop_rate; 
	}
	else if (current_pos < target) {
		current_pos += rate / loop_rate;
	}
	} 
}

void setpoint_check() {
	if (mag > range)
	{
		ROS_WARN("Desired angle is greater than %f deg, limit to %f deg", range, range);
		mag = range;
		ros::param::set("gimbal_ctrl/mag", range);
	}
	else if (mag < 0.0)
	{
		ROS_WARN("Desired angle is smaller than 0 deg, limit to 0 deg");
		mag = 0.0;
		ros::param::set("gimbal_ctrl/mag", 0.0);
	}
}


int main(int argc, char** argv) {

	ros::init(argc, argv, "px4_gimbal_ctrl");
	ros::NodeHandle nh("");
	
	/* Subscribers */
	ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
    				("mavros/state", 10, state_cb);
				   
	ros::Subscriber gimbal_pos_sub = nh.subscribe<geometry_msgs::Quaternion>
					("/mavros/mount_control/orientation", 10, gimbal_pos_cb);
	
	/* Publishers */
	ros::Publisher dataPublisher = nh.advertise<mavros_msgs::MountControl>
		 ("mavros/mount_control/command", 10);

	/* Servicecs */
	ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
	            ("mavros/set_mode");

	/* handle params*/
	ros::param::get("gimbal_ctrl/freq", freq);
	ros::param::get("gimbal_ctrl/type", type);
	ros::param::get("gimbal_ctrl/bias", bias);
	ros::param::get("gimbal_ctrl/range", range);

	ros::param::get("gimbal_ctrl/mag", mag);
	
	setpoint_check();

	// Create a rate
	ros::Rate rate(loop_rate);

	// wait for FCU connection
	// while (ros::ok() && !current_state.connected) {
	// 	ros::spinOnce();
	// 	rate.sleep();
	// }
	
	/* set offboard mode */
	mavros_msgs::SetMode offb_set_mode;
	offb_set_mode.request.custom_mode = "OFFBOARD";

	ros::Time last_request = ros::Time::now();

	while (ros::ok()) {
		ros::param::get("gimbal_ctrl/mag", mag);
		setpoint_check();
		mag = (mag < 0.0) ? 0.0 : ((mag > range) ? range : mag);

		/* handle current mode */
		if( current_state.mode != "OFFBOARD" && (ros::Time::now() - last_request > ros::Duration(5.0))) {
			if( set_mode_client.call(offb_set_mode) && offb_set_mode.response.mode_sent) {
		                ROS_INFO("Offboard enabled");
			}
			last_request = ros::Time::now();
		}	    

        	ros::Time current_time = ros::Time::now();

        	mavros_msgs::MountControl msg;

        	msg.header.stamp = ros::Time::now();
        	msg.header.frame_id = "body";
        	msg.mode = 2;

			if (type == "sine") {
				msg.roll = sin(freq* M_PI * ros::Time::now().toSec()) * mag; //in degrees 阀门
				msg.pitch = sin(freq* M_PI * ros::Time::now().toSec()) * mag; //in degrees 气泵
			}
			else if (type == "fixed") {
				// run planner
				if (last_pos_updated) {
				position_planner(mag, position);
				msg.roll = position + bias;
				}
			} 
			else {
				msg.roll = 0.0;
			} 
        	
		if (last_pos_updated) {
			dataPublisher.publish(msg);
		}
		ros::spinOnce();
		rate.sleep();
	}

	return 0;
}
