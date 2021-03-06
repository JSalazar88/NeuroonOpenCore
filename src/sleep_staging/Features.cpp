/*
 * EegFeatures.cpp
 *
 *  Created on: Jul 28, 2016
 *      Author: tomek
 */

#include "Features.h"
#include "Spectrogram.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <exception>
#include <functional>
#include "dlib_utils.h"

Features::Features() {
	// TODO Auto-generated constructor stub

}

Features::~Features() {
	// TODO Auto-generated destructor stub
}

dlib::matrix<double> Features::sum_in_band(const Spectrogram& s, double low, double high, bool normalized) {
  dlib::matrix<double> band = s.get_band(low, high);
  dlib::matrix<double> sum = dlib::sum_cols(band);

  if (normalized) {
	  sum = (1. / band.nc()) * sum;
  }
  return sum;
}

std::vector<std::pair<double, double>> Features::create_bands(const std::vector<double>& borders) {
	std::vector<std::pair<double,double>> result(borders.size() - 1);
	for (std::size_t i = 0; i != borders.size() - 1; ++i) {
		result[i] = std::pair<double, double>(borders[i], borders[i+1]);
	}
	return result;
}


dlib::matrix<double> Features::sum_in_bands(const Spectrogram& s, const std::vector<std::pair<double, double>> &bands, bool normalized) {
	dlib::matrix<double> result(s.size(), bands.size());

	for (std::size_t i = 0; i != bands.size(); ++i) {
		dlib::set_colm(result, i) = sum_in_band(s, bands[i].first, bands[i].second, normalized);
	}

	return result;
}

dlib::matrix<double> Features::sum_by_borders(const Spectrogram& s, const std::vector<double> &borders, bool normalized) {
	auto bands = create_bands(borders);
	return sum_in_bands(s, bands, normalized);
}

//centered, inserts NaNs in the beginning and in the end
dlib::matrix<double> Features::rolling_mean(const dlib::matrix<double> &signal, int window_size) {

	if (signal.nr() < window_size) {
		throw std::logic_error("rolling_mean: window bigger than signal!");
	}

	dlib::matrix<double> result(signal.nr(), 1);
	dlib::set_all_elements(result, NAN);
	for (size_t i = 0; i != signal.nr() - window_size; ++i) {
		dlib::matrix<double> window = dlib::rowm(signal, dlib::range(i, i+window_size-1));

		double sum = dlib::sum(window);
		double mean = sum / window_size;
		result(i + (window_size/2), 0) = mean;
	}
	return result;
}

dlib::matrix<double> Features::rolling_std(const dlib::matrix<double> &signal, int window_size) {
	dlib::matrix<double> result(signal.nr(), 1);
	dlib::set_all_elements(result, NAN);
	for (size_t i = 0; i != signal.nr() - window_size; ++i) {
		const dlib::matrix<double> window = dlib::rowm(signal, dlib::range(i, i+window_size-1));

		double sd = standard_deviation(window);
		result(i + (window_size/2), 0) = sd;
	}
	return result;
}



//doesn't take NaNs into consideration, takes neighbouring values instead
dlib::matrix<double> Features::sparse_rolling(const dlib::matrix<double> &signal, int window_size,
												 std::function<dlib::matrix<double> (const dlib::matrix<double>&, int)> rolling_operation) {

	dlib::matrix<double> index(signal.nr(), 1);
	dlib::set_colm(index, 0) = dlib::trans(dlib::range(0, signal.nr() - 1));
	dlib::matrix<int> nonnans = nonnan_rows(signal);

	dlib::matrix<double> result(signal.nr(), signal.nc());
	dlib::set_all_elements(result, NAN);

	if (nonnans.size() > 0) {
		dlib::matrix<double> rolling_input = dlib::rowm(signal, nonnans);
		dlib::set_rowm(result, nonnans) = rolling_operation(rolling_input, window_size);
	}

	return result;
}

dlib::matrix<double> Features::sparse_rolling_mean(const dlib::matrix<double> &signal, int window_size) {
	return Features::sparse_rolling(signal, window_size, rolling_mean);
}

dlib::matrix<double> Features::sparse_rolling_std(const dlib::matrix<double> &signal, int window_size) {
	return Features::sparse_rolling(signal, window_size, rolling_std);
}

double median_sorted(const dlib::matrix<double> &signal) {
	if (signal.nr() % 2 == 1) {
		return signal( signal.nr() / 2, 0);
	} else {
		double m1 = signal((signal.nr() / 2), 0);
		double m2 = signal((signal.nr() / 2) + 1, 0);
		return (m1 + m2) / 2;
	}
}

dlib::matrix<double> Features::n_max_to_median(const dlib::matrix<double> &data, int n) {
	dlib::matrix<double> input = data;
	dlib::matrix<double> result(data.nr(), 1);

	for (std::size_t i = 0; i != data.nr(); ++i) {
		dlib::matrix<double> row = dlib::rowm(input, i);
		std::sort(row.begin(), row.end());

		double median = median_sorted(dlib::trans(row));
		dlib::matrix<double> n_max = dlib::colm(row, dlib::range(row.nc() - n, row.nc() - 1));
		double n_max_sum = dlib::sum(n_max);
		result(i, 0) = n_max_sum / median;
	}

	return result;
}


dlib::matrix<double> Features::standardize(const dlib::matrix<double> &signal) {
	dlib::matrix<int> nonnans = nonnan_rows(signal);

	if (nonnans.nr() == 0) {
		dlib::matrix<double> result(signal.nr(), signal.nc());
		dlib::set_all_elements(result, NAN);
		return result;
	} else {
		dlib::matrix<double> correct_rows = dlib::rowm(signal, nonnans);
		double mean = dlib::mean(correct_rows);
		double sd = standard_deviation(correct_rows);
		std::cout << "mean: " << mean << "; std: " << sd << std::endl;
		dlib::matrix<double> result = (signal - mean) / sd;
		return result;
	}
}

dlib::matrix<double> Features::standardize_in_window(const dlib::matrix<double> &signal, int window_size) {
	dlib::matrix<double> rolling_mean = sparse_rolling_mean(signal, window_size);
	dlib::matrix<double> rolling_std = sparse_rolling_std(signal, window_size);

	//throw std::logic_error("Not implemented");
	//TODO: don't return dummy result
	dlib::matrix<double> result(signal.nr(), 1);
	result = signal;
	result = result - rolling_mean;

	//no poinwise divide in dlib
	for (int i = 0; i != result.nr(); ++i) {
		result(i, 0) = result(i, 0) / rolling_std(i, 0);
	}

	return result;
}
