#include <gtest/gtest.h>
#include <dlib/matrix.h>
#include "functional_tests_data.h"
#include "BrainWaveLevels.h"
#include "Spectrogram.h"

TEST(BrainWaveLevelsFunctional, basic_functional) {
	dlib::matrix<double> eeg(get_eeg_data());
	dlib::matrix<double> small_eeg = dlib::rowm(eeg, dlib::range(1, 1024 * 10));

	Spectrogram eeg_spectrogram(small_eeg, 125, 256, 128);
	BrainWaveLevels bw;

	std::vector<brain_wave_levels_t> result = bw.predict(eeg_spectrogram);
	ASSERT_TRUE(result.size() == eeg_spectrogram.size());

	for (int i = 0; i != result.size(); ++i) {
		brain_wave_levels_t l = result[i];
		ASSERT_TRUE(l.alpha < 1);
		ASSERT_TRUE(l.alpha > 0);

		ASSERT_TRUE(l.beta < 1);
		ASSERT_TRUE(l.beta > 0);

		ASSERT_TRUE(l.delta < 1);
		ASSERT_TRUE(l.delta > 0);

		ASSERT_TRUE(l.theta < 1);
		ASSERT_TRUE(l.theta > 0);
	}
}
