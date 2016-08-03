#include "csp_solver_util.h"
#include <math.h>

C_csp_reported_outputs::C_output::C_output()
{
	mp_reporting_ts_array = 0;		// Initialize pointer to NULL

	m_is_allocated = false;

	m_counter_reporting_ts_array = 0;

	m_n_reporting_ts_array = -1;
}

void C_csp_reported_outputs::C_output::allocate(float *p_reporting_ts_array, int n_reporting_ts_array)
{
	mp_reporting_ts_array = p_reporting_ts_array;
	mv_temp_outputs.reserve(10);

	m_is_allocated = true;

	m_n_reporting_ts_array = n_reporting_ts_array;
}

void C_csp_reported_outputs::C_output::set_m_is_ts_weighted(bool is_ts_weighted)
{
	m_is_ts_weighted = is_ts_weighted;
}

int C_csp_reported_outputs::C_output::get_vector_size()
{
	return mv_temp_outputs.size();
}

void C_csp_reported_outputs::C_output::set_timestep_output(double output_value)
{
	if( m_is_allocated )
	{	
		mv_temp_outputs.push_back(output_value);
	}
}

void C_csp_reported_outputs::C_output::send_to_reporting_ts_array(double report_time_start, int n_report, 
		const std::vector<double> & v_temp_ts_time_end, double report_time_end, bool is_save_last_step, int n_pop_back )
{
	if( m_is_allocated )
	{	
		if( mv_temp_outputs.size() != n_report )
		{
			throw(C_csp_exception("Time and data arrays are not the same size", "C_csp_reported_outputs::send_to_reporting_ts_array"));
		}

		if( m_counter_reporting_ts_array + 1 > m_n_reporting_ts_array )
		{
			throw(C_csp_exception("Attempting store more points in Reporting Timestep Array than it was allocated for"));
		}
	
		double m_report_step = report_time_end - report_time_start;

		if( m_is_ts_weighted )
		{	// ***********************************************************
			//      Set outputs that are reported as weighted averages if 
			//       multiple csp-timesteps for one reporting timestep
			//    The following code assumes 'SOLZEN' is the first such output
			//    and that all names following it in 'E_reported_outputs' are weight averages
			// **************************************************************			
			double time_prev = report_time_start;		//[s]
			for( int i = 0; i < n_report; i++ )
			{
				mp_reporting_ts_array[m_counter_reporting_ts_array] += (fmin(v_temp_ts_time_end[i], report_time_end) - time_prev)*mv_temp_outputs[i];	//[units]*[s]
				time_prev = fmin(v_temp_ts_time_end[i], report_time_end);
			}
			mp_reporting_ts_array[m_counter_reporting_ts_array] /= m_report_step;
		}
		else
		{	// ************************************************************
			// Set instantaneous outputs that are reported as the first value
			//   if multiple csp-timesteps for one reporting timestep
			// ************************************************************
			mp_reporting_ts_array[m_counter_reporting_ts_array] = mv_temp_outputs[0];
		}

		if( is_save_last_step )
		{
			mv_temp_outputs[0] = mv_temp_outputs[n_report - 1];
		}

		for( int i = 0; i < n_pop_back; i++ )
		{
			mv_temp_outputs.pop_back();
		}

		m_counter_reporting_ts_array++;

	}	
}

void C_csp_reported_outputs::send_to_reporting_ts_array(double report_time_start,
	const std::vector<double> & v_temp_ts_time_end, double report_time_end)
{
	int n_report = v_temp_ts_time_end.size();
	if( n_report < 1 )
	{
		throw(C_csp_exception("No data to report", "C_csp_reported_outputs::send_to_reporting_ts_array"));
	}

	bool is_save_last_step = true;
	int n_pop_back = n_report - 1;
	if( v_temp_ts_time_end[n_report - 1] == report_time_end )
	{
		is_save_last_step = false;
		n_pop_back = n_report;
	}

	for( int i = 0; i < m_n_outputs; i++ )
	{
		mvc_outputs[i].send_to_reporting_ts_array(report_time_start, n_report, v_temp_ts_time_end,
			report_time_end, is_save_last_step, n_pop_back);
	}
}

void C_csp_reported_outputs::construct(const S_output_info *output_info, int n_outputs)
{
	// Resize the vector of Output Classes
	mvc_outputs.resize(n_outputs);
	m_n_outputs = n_outputs;

	mv_latest_calculated_outputs.resize(n_outputs);

	// Loop through the output info and set m_is_ts_weighted for each output
	for( int i = 0; i < n_outputs; i++ )
	{
		mvc_outputs[i].set_m_is_ts_weighted(output_info[i].m_is_ts_weighted);
	}

	m_n_reporting_ts_array = -1;
}

bool C_csp_reported_outputs::allocate(int index, float *p_reporting_ts_array, int n_reporting_ts_array)
{
	if(index < 0 || index >= m_n_outputs)
		return false;

	if(m_n_reporting_ts_array == -1)
	{
		m_n_reporting_ts_array = n_reporting_ts_array;
	}
	else
	{
		if( m_n_reporting_ts_array != n_reporting_ts_array )
			return false;
	}

	mvc_outputs[index].allocate(p_reporting_ts_array, n_reporting_ts_array);

	return true;
}

void C_csp_reported_outputs::set_timestep_outputs()
{
	for(int i = 0; i < m_n_outputs; i++)
		mvc_outputs[i].set_timestep_output(mv_latest_calculated_outputs[i]);
}

void C_csp_reported_outputs::value(int index, double value)
{
	mv_latest_calculated_outputs[index] = value;
}

double C_csp_reported_outputs::value(int index)
{
	return mv_latest_calculated_outputs[index];
}

C_csp_messages::C_csp_messages()
{
	//m_message_list.resize(0);
    m_message_list.clear();
}

void C_csp_messages::add_message(int type, std::string msg)
{
	// Want first message last...
    m_message_list.insert( m_message_list.begin(), S_message_def(type, msg) );

}

void C_csp_messages::add_notice(std::string msg)
{
	add_message(C_csp_messages::NOTICE, msg);
}

bool C_csp_messages::get_message(int *type, std::string *msg)
{
    if(m_message_list.size() == 0)
        return false;

    S_message_def temp = m_message_list.back();
    m_message_list.pop_back();

    *msg = temp.msg;
    *type = temp.m_type;

    return true;
}

bool C_csp_messages::get_message(std::string *msg)
{
    int itemp;

    return get_message(&itemp, msg);
}

const char* C_csp_exception::what()
{
	return "CSP exception";
}

C_csp_exception::C_csp_exception(const char *cmsg)
{
	m_error_message = cmsg;
	m_code_location = "unknown";
	m_error_code = -1;
}
C_csp_exception::C_csp_exception(const std::string &error_message, const std::string &code_location)
{
	m_error_message = error_message;
	m_code_location = code_location;
	m_error_code = -1;
}
C_csp_exception::C_csp_exception(const std::string &error_message, const std::string &code_location, int error_code)
{
	m_error_message = error_message;
	m_code_location = code_location;
	m_error_code = error_code;
}

bool check_double(double x)
{
	if( x != x )
	{
		return false;
	}
	return true;
}
