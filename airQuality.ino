/*
soglie:
iaq:
  eccellente : 0-50
  buono/medio: 51-100
  leggermente inquinata 101-150
  inquinamento moderato: 151-200
  gravemente inquinata 201-300
  estremamente inquinata 301-500+

eco2:
  ottimo: 400 - 600ppm
  accettabile: 600 - 1000 ppm
  mediocre 1000 - 1500
  pessimo 1500+

bvoc:
  pulito 0.5-1 
  moderato 1-3
  elevato 3-10
  critico 10+

gas:
  ottimo 0 - 5%
  accettabile 5.1 - 15%
  mediocre 15.1 - 25%
  pessimo 25.1 - 100
*/

float normalizeValue(float val, float minVal, float maxVal)
{
  if (val <= minVal) return 0.0f;
  if (val >= maxVal) return 1.0f;
  return (val - minVal) / (maxVal - minVal);
}

void calcAirQuality(void)
{
    if (ambData.bsec2.status.accuracy == 0) {
    ambData.airQuality = AIR_UNKNOWN;
    return;
  }
  float n_iaq = normalizeValue(ambData.bsec2.debug.IAQ, 0.0f, 300.0f);
  float n_eco2 = normalizeValue(ambData.bsec2.debug.eCO2, 400.0f, 1500.0f);
  float n_bvoc = normalizeValue(ambData.bsec2.debug.bVOC, 0.5f, 10.0f);
  float n_gasPerc = normalizeValue(ambData.bsec2.debug.gasPerc, 0.0f, 25.0f);

  const float w_iaq = 0.40f;    // weight for IAQ
  const float w_eco2 = 0.30f;   // weight for equivalent CO2
  const float w_bvoc = 0.15f;   // weight for organic volatile particles
  const float w_gas = 0.15f;    // weight for gas in air

  float globalScore = (n_iaq * w_iaq) + (n_eco2 * w_eco2) + (n_bvoc * w_bvoc) + (n_gasPerc * w_gas);

  if (globalScore <= 0.12f) { ambData.airQuality = AIR_EXCELLENT; }
  else if (globalScore <= 0.30f) { ambData.airQuality = AIR_GOOD; }
  else if (globalScore <= 0.50f) { ambData.airQuality = AIR_FAIR; }
  else if (globalScore <= 0.70f) { ambData.airQuality = AIR_POOR; }
  else if (globalScore <= 0.85f) { ambData.airQuality = AIR_BAD; }
  else { ambData.airQuality = AIR_VERY_BAD; }
}