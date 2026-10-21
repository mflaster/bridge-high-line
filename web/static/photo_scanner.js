// =========================================================================
// On-Device Bridge Hand Record OCR & Interactive Crop Modal
// =========================================================================
const OCR_TEMPLATE_DATA = {"aspect":{"2":0.739,"3":0.703,"4":0.78,"5":0.712,"6":0.718,"7":0.703,"8":0.728,"9":0.731,"K":0.902,"J":0.6,"A":1.016,"Q":0.985},"tpl":{"2":[0.031,0.09,0.118,0.092,0.051,0.02,0.008,0.049,0.098,0.119,0.109,0.061,0.061,0.114,0.118,0.055,0.003,0.001,0,0.007,0.056,0.107,0.119,0.091,0.079,0.119,0.108,0.034,0,0,0,0,0.027,0.087,0.12,0.109,0.049,0.08,0.059,0.012,0,0,0,0,0.023,0.083,0.12,0.112,0.002,0.003,0.002,0.001,0,0,0,0,0.045,0.105,0.12,0.095,0,0,0,0,0,0,0,0.015,0.081,0.119,0.119,0.074,0,0,0,0,0,0,0.001,0.049,0.112,0.119,0.095,0.036,0,0,0,0,0,0.017,0.053,0.098,0.109,0.078,0.033,0.004,0,0,0,0,0.023,0.08,0.116,0.108,0.058,0.016,0.001,0,0,0,0,0.022,0.082,0.115,0.12,0.071,0.014,0.001,0,0,0,0,0.011,0.069,0.12,0.118,0.088,0.022,0,0,0,0,0.004,0.031,0.074,0.107,0.095,0.059,0.019,0,0,0,0,0,0.035,0.095,0.119,0.091,0.023,0.001,0,0,0,0,0,0,0.075,0.12,0.12,0.097,0.06,0.06,0.06,0.06,0.06,0.06,0.06,0.051,0.103,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.111,0.118,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12],"3":[0.053,0.112,0.126,0.093,0.044,0.011,0.033,0.088,0.126,0.119,0.069,0.013,0.085,0.126,0.113,0.044,0,0,0,0.036,0.11,0.126,0.1,0.037,0.1,0.125,0.093,0.02,0,0,0,0.015,0.086,0.126,0.111,0.048,0.034,0.041,0.028,0.005,0,0,0,0.018,0.088,0.126,0.105,0.042,0,0,0,0,0.001,0.001,0.014,0.067,0.117,0.122,0.075,0.016,0,0,0,0,0.032,0.064,0.076,0.113,0.123,0.088,0.028,0,0,0,0,0.005,0.07,0.12,0.121,0.126,0.123,0.077,0.025,0.008,0,0,0,0.003,0.036,0.052,0.056,0.077,0.111,0.121,0.101,0.051,0,0,0,0,0,0,0,0.01,0.07,0.118,0.126,0.092,0,0,0,0,0,0,0,0,0.029,0.092,0.126,0.111,0.049,0.049,0.028,0.002,0,0,0,0,0.015,0.078,0.126,0.119,0.114,0.117,0.073,0.01,0,0,0,0,0.029,0.092,0.126,0.11,0.093,0.124,0.109,0.05,0.009,0,0,0.022,0.079,0.12,0.121,0.078,0.04,0.101,0.125,0.106,0.065,0.049,0.049,0.087,0.122,0.122,0.085,0.027,0.011,0.049,0.098,0.12,0.114,0.112,0.112,0.121,0.115,0.083,0.032,0.001,0.001,0.008,0.041,0.074,0.084,0.109,0.096,0.077,0.059,0.025,0.004,0],"4":[0,0,0,0,0,0.029,0.1,0.13,0.129,0.066,0.002,0,0,0,0,0,0.005,0.066,0.125,0.129,0.129,0.066,0.003,0,0,0,0,0.001,0.032,0.102,0.116,0.117,0.129,0.067,0.003,0,0,0,0,0.03,0.093,0.109,0.061,0.087,0.129,0.067,0.002,0,0,0,0.008,0.08,0.126,0.067,0.01,0.077,0.13,0.066,0.002,0,0,0.001,0.037,0.113,0.105,0.032,0.006,0.077,0.13,0.066,0.002,0,0,0.013,0.08,0.119,0.062,0.007,0.007,0.077,0.13,0.066,0.002,0,0.018,0.069,0.114,0.071,0.013,0,0.007,0.078,0.13,0.066,0.002,0,0.066,0.119,0.094,0.016,0,0,0.007,0.077,0.13,0.066,0.001,0,0.105,0.129,0.101,0.064,0.064,0.064,0.068,0.103,0.13,0.097,0.064,0.06,0.127,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.126,0.083,0.087,0.087,0.087,0.087,0.087,0.089,0.112,0.13,0.108,0.087,0.084,0.002,0.002,0.002,0.002,0.002,0.002,0.009,0.078,0.13,0.067,0.003,0.002,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0.001,0,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0,0,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0,0],"5":[0.007,0.067,0.121,0.105,0.067,0.06,0.06,0.06,0.06,0.06,0.06,0.03,0.022,0.084,0.123,0.08,0.009,0.002,0.002,0.002,0.002,0.002,0.002,0.001,0.031,0.093,0.123,0.069,0,0,0,0,0,0,0,0,0.044,0.105,0.121,0.056,0.008,0.032,0.042,0.022,0.001,0,0,0,0.066,0.121,0.12,0.085,0.071,0.107,0.122,0.093,0.061,0.031,0.001,0,0.078,0.123,0.123,0.122,0.12,0.121,0.121,0.122,0.122,0.093,0.038,0.006,0.087,0.123,0.119,0.092,0.057,0.05,0.05,0.077,0.114,0.123,0.102,0.046,0.054,0.073,0.061,0.025,0.003,0,0,0.011,0.06,0.11,0.123,0.097,0,0,0,0,0,0,0,0,0.019,0.081,0.123,0.119,0,0,0,0,0,0,0,0,0.005,0.067,0.123,0.123,0.028,0.028,0.016,0.001,0,0,0,0,0.004,0.066,0.123,0.123,0.1,0.102,0.063,0.008,0,0,0,0,0.022,0.083,0.123,0.11,0.103,0.123,0.101,0.037,0.005,0,0,0.012,0.065,0.113,0.12,0.075,0.063,0.114,0.12,0.087,0.041,0.027,0.027,0.063,0.109,0.121,0.094,0.035,0.024,0.073,0.108,0.115,0.095,0.089,0.089,0.109,0.113,0.088,0.045,0.008,0.004,0.021,0.063,0.095,0.099,0.11,0.104,0.096,0.073,0.033,0.008,0],"6":[0.011,0.059,0.105,0.101,0.066,0.027,0.006,0.045,0.094,0.114,0.101,0.046,0.035,0.092,0.114,0.072,0.012,0.001,0,0.007,0.057,0.105,0.113,0.067,0.061,0.11,0.106,0.042,0,0,0,0,0.023,0.079,0.112,0.069,0.084,0.114,0.077,0.014,0.001,0.001,0.001,0.001,0.003,0.021,0.038,0.021,0.103,0.114,0.06,0.018,0.05,0.056,0.056,0.056,0.044,0.016,0,0,0.111,0.111,0.068,0.062,0.105,0.11,0.11,0.112,0.1,0.07,0.026,0,0.113,0.112,0.098,0.097,0.084,0.056,0.052,0.085,0.108,0.112,0.079,0.027,0.113,0.114,0.108,0.06,0.016,0.003,0.002,0.018,0.064,0.105,0.11,0.073,0.112,0.114,0.094,0.025,0,0,0,0,0.02,0.078,0.114,0.1,0.111,0.114,0.08,0.015,0,0,0,0,0.006,0.063,0.114,0.111,0.099,0.114,0.077,0.013,0,0,0,0,0.006,0.062,0.113,0.11,0.08,0.114,0.094,0.026,0,0,0,0,0.017,0.073,0.114,0.1,0.052,0.103,0.109,0.059,0.008,0,0,0.009,0.055,0.101,0.11,0.069,0.019,0.068,0.106,0.1,0.055,0.026,0.026,0.05,0.099,0.112,0.088,0.034,0.004,0.029,0.07,0.106,0.099,0.084,0.084,0.097,0.108,0.091,0.05,0.01,0,0.004,0.021,0.067,0.087,0.099,0.102,0.089,0.075,0.045,0.014,0],"7":[0.084,0.084,0.084,0.084,0.084,0.084,0.084,0.088,0.13,0.169,0.167,0.133,0.006,0.006,0.006,0.006,0.006,0.006,0.008,0.041,0.124,0.167,0.138,0.065,0,0,0,0,0,0.003,0.019,0.092,0.161,0.152,0.084,0.017,0,0,0,0,0.001,0.015,0.081,0.151,0.151,0.087,0.021,0.001,0,0,0,0,0.004,0.052,0.147,0.168,0.101,0.017,0,0,0,0,0,0.002,0.008,0.085,0.167,0.144,0.057,0.001,0,0,0,0,0.001,0.006,0.024,0.116,0.166,0.105,0.015,0,0,0,0,0,0.004,0.014,0.075,0.153,0.138,0.052,0,0,0,0,0,0,0.006,0.039,0.126,0.165,0.095,0.01,0,0,0,0,0,0.001,0.006,0.06,0.148,0.151,0.062,0,0,0,0,0,0,0.004,0.01,0.081,0.16,0.13,0.035,0,0,0,0,0,0.001,0.006,0.029,0.114,0.163,0.1,0.009,0,0,0,0,0,0.003,0.007,0.048,0.135,0.163,0.075,0,0,0,0,0,0,0.004,0.008,0.059,0.144,0.161,0.066,0,0,0,0,0,0,0.008,0.01,0.071,0.154,0.159,0.061,0,0,0,0,0,0,0.011,0.011,0.084,0.161,0.152,0.054,0,0,0,0,0,0],"8":[0.02,0.074,0.111,0.09,0.042,0.015,0.012,0.059,0.106,0.111,0.074,0.019,0.044,0.1,0.113,0.058,0.002,0.001,0.001,0.021,0.085,0.113,0.1,0.044,0.057,0.112,0.112,0.041,0,0,0,0.004,0.062,0.113,0.111,0.055,0.05,0.106,0.112,0.048,0,0,0,0.007,0.067,0.113,0.109,0.053,0.022,0.078,0.113,0.086,0.025,0.001,0.001,0.043,0.097,0.112,0.082,0.026,0.003,0.032,0.085,0.11,0.082,0.057,0.057,0.091,0.112,0.084,0.03,0.002,0.002,0.011,0.064,0.112,0.11,0.101,0.102,0.111,0.109,0.062,0.011,0.002,0.03,0.072,0.101,0.088,0.058,0.034,0.037,0.069,0.098,0.1,0.072,0.027,0.077,0.113,0.103,0.04,0,0,0,0.009,0.062,0.105,0.113,0.071,0.102,0.113,0.081,0.017,0,0,0,0,0.022,0.079,0.113,0.097,0.112,0.113,0.066,0.006,0,0,0,0,0.006,0.062,0.113,0.109,0.104,0.113,0.076,0.013,0,0,0,0,0.012,0.069,0.113,0.107,0.074,0.11,0.103,0.04,0.004,0,0,0.006,0.049,0.098,0.112,0.081,0.038,0.091,0.113,0.082,0.038,0.023,0.023,0.046,0.096,0.113,0.096,0.043,0.01,0.054,0.095,0.104,0.089,0.081,0.081,0.094,0.11,0.096,0.055,0.013,0,0.016,0.046,0.079,0.089,0.101,0.101,0.089,0.078,0.048,0.014,0],"9":[0.035,0.09,0.113,0.09,0.045,0.016,0.012,0.059,0.105,0.108,0.066,0.014,0.065,0.113,0.11,0.052,0.002,0,0.001,0.016,0.078,0.113,0.098,0.042,0.084,0.113,0.094,0.025,0,0,0,0.001,0.044,0.099,0.113,0.068,0.107,0.113,0.069,0.008,0,0,0,0,0.019,0.076,0.113,0.093,0.112,0.113,0.065,0.006,0,0,0,0,0.019,0.076,0.113,0.106,0.101,0.113,0.085,0.019,0,0,0,0,0.039,0.095,0.113,0.11,0.079,0.113,0.106,0.04,0.004,0,0,0.009,0.065,0.11,0.113,0.113,0.04,0.093,0.113,0.089,0.054,0.044,0.044,0.066,0.096,0.106,0.113,0.112,0.003,0.031,0.084,0.113,0.113,0.113,0.113,0.103,0.06,0.075,0.113,0.107,0,0,0.025,0.063,0.095,0.102,0.079,0.044,0.019,0.063,0.113,0.102,0,0,0,0.009,0.033,0.038,0.019,0.001,0.024,0.081,0.113,0.087,0.029,0.045,0.038,0.011,0,0,0,0.001,0.049,0.104,0.113,0.064,0.065,0.111,0.106,0.038,0.001,0,0.001,0.024,0.086,0.113,0.094,0.037,0.039,0.094,0.113,0.071,0.021,0.018,0.021,0.069,0.111,0.1,0.052,0.008,0.013,0.06,0.104,0.101,0.077,0.076,0.077,0.101,0.103,0.062,0.015,0,0,0.019,0.062,0.092,0.101,0.108,0.1,0.09,0.06,0.019,0,0],"K":[0.126,0.094,0.014,0,0,0.001,0.025,0.092,0.125,0.093,0.024,0,0.126,0.094,0.015,0,0,0.016,0.075,0.118,0.114,0.051,0,0,0.126,0.094,0.015,0,0.002,0.051,0.115,0.113,0.069,0.015,0,0,0.126,0.094,0.015,0.01,0.051,0.103,0.112,0.059,0.011,0,0,0,0.126,0.094,0.015,0.048,0.115,0.112,0.054,0.007,0,0,0,0,0.126,0.095,0.037,0.091,0.126,0.099,0.022,0,0,0,0,0,0.126,0.111,0.093,0.118,0.124,0.113,0.044,0,0,0,0,0,0.126,0.126,0.126,0.088,0.072,0.115,0.102,0.036,0,0,0,0,0.126,0.126,0.096,0.024,0.008,0.083,0.126,0.083,0.008,0,0,0,0.126,0.112,0.043,0.001,0,0.057,0.126,0.109,0.037,0,0,0,0.126,0.096,0.017,0,0,0.024,0.098,0.125,0.086,0.017,0,0,0.126,0.094,0.014,0,0,0.002,0.029,0.099,0.122,0.059,0.001,0,0.126,0.094,0.014,0,0,0,0,0.064,0.126,0.102,0.027,0,0.126,0.093,0.014,0,0,0,0,0.03,0.103,0.123,0.081,0.023,0.126,0.093,0.014,0,0,0,0,0.005,0.061,0.119,0.118,0.065,0.126,0.094,0.015,0,0,0,0,0,0.021,0.098,0.126,0.105],"J":[0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0.005,0.006,0.002,0,0,0,0,0,0.06,0.122,0.124,0.124,0.047,0.06,0.023,0.001,0,0,0,0,0.06,0.122,0.124,0.123,0.109,0.112,0.053,0.002,0,0,0,0.004,0.065,0.123,0.124,0.118,0.115,0.124,0.08,0.023,0.006,0.002,0.005,0.032,0.089,0.124,0.116,0.096,0.095,0.121,0.111,0.085,0.047,0.021,0.048,0.09,0.114,0.123,0.09,0.049,0.058,0.105,0.12,0.121,0.1,0.081,0.102,0.123,0.122,0.107,0.053,0.01,0.018,0.065,0.098,0.105,0.112,0.117,0.109,0.105,0.091,0.056,0.017,0],"A":[0,0,0,0.011,0.094,0.132,0.134,0.072,0,0,0,0,0,0,0,0.023,0.109,0.114,0.128,0.084,0.002,0,0,0,0,0,0.001,0.05,0.123,0.074,0.101,0.104,0.015,0,0,0,0,0,0.012,0.09,0.124,0.038,0.062,0.12,0.05,0,0,0,0,0,0.036,0.124,0.099,0.012,0.027,0.113,0.079,0,0,0,0,0,0.046,0.132,0.085,0.004,0.016,0.105,0.105,0.025,0,0,0,0.001,0.066,0.134,0.067,0.001,0.004,0.082,0.13,0.052,0,0,0,0.01,0.096,0.121,0.037,0,0,0.058,0.133,0.08,0.008,0,0,0.032,0.121,0.122,0.076,0.062,0.062,0.087,0.134,0.115,0.026,0,0,0.045,0.132,0.133,0.131,0.131,0.131,0.132,0.134,0.127,0.05,0,0.001,0.062,0.132,0.108,0.094,0.097,0.099,0.102,0.125,0.134,0.071,0,0.006,0.088,0.104,0.031,0.019,0.021,0.022,0.024,0.069,0.13,0.093,0.012,0.034,0.113,0.074,0.001,0,0,0,0,0.019,0.101,0.117,0.042,0.068,0.132,0.055,0,0,0,0,0,0.002,0.077,0.128,0.076,0.092,0.132,0.044,0,0,0,0,0,0,0.064,0.133,0.108,0.122,0.118,0.028,0,0,0,0,0,0,0.04,0.122,0.124],"Q":[0.002,0.046,0.128,0.108,0.057,0.013,0.009,0.055,0.113,0.127,0.048,0,0.005,0.079,0.129,0.058,0.002,0,0,0.004,0.057,0.127,0.088,0.013,0.041,0.118,0.089,0.012,0,0,0,0,0.01,0.085,0.124,0.053,0.08,0.136,0.05,0,0,0,0,0,0,0.047,0.132,0.091,0.1,0.13,0.037,0,0,0,0,0,0,0.032,0.123,0.101,0.122,0.121,0.029,0,0,0,0,0,0,0.025,0.117,0.12,0.137,0.11,0.017,0,0,0,0,0,0,0.023,0.116,0.128,0.124,0.119,0.027,0,0,0,0,0,0,0.024,0.116,0.121,0.102,0.13,0.037,0,0,0,0,0,0,0.03,0.122,0.104,0.083,0.135,0.046,0,0,0,0,0,0,0.044,0.133,0.094,0.046,0.12,0.082,0.008,0,0.01,0.066,0.061,0.022,0.079,0.129,0.061,0.008,0.083,0.123,0.049,0,0.014,0.093,0.132,0.1,0.124,0.093,0.017,0.002,0.048,0.129,0.106,0.051,0.01,0.04,0.121,0.136,0.134,0.055,0,0,0.011,0.066,0.123,0.126,0.094,0.094,0.126,0.121,0.119,0.089,0.042,0,0,0.006,0.052,0.079,0.091,0.088,0.079,0.048,0.061,0.126,0.126,0,0,0,0.006,0.01,0.022,0.019,0.01,0.005,0.018,0.079,0.109]}};

const OCR_RANKS = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2'];
const OCR_TW = 12, OCR_TH = 16;

function computeBinaryMask(img) {
  const { width, height, data } = img;
  const stride = data.length === width * height * 4 ? 4 : 3;
  const gray = new Float32Array(width * height);
  const intImg = new Float64Array((width + 1) * (height + 1));
  for (let y = 0; y < height; y++) {
    let rowSum = 0;
    for (let x = 0; x < width; x++) {
      const p = (y * width + x) * stride;
      const lum = 0.299 * data[p] + 0.587 * data[p + 1] + 0.114 * data[p + 2];
      gray[y * width + x] = lum;
      rowSum += lum;
      intImg[(y + 1) * (width + 1) + (x + 1)] =
          intImg[y * (width + 1) + (x + 1)] + rowSum;
    }
  }
  const mask = new Uint8Array(width * height);
  const rad = Math.max(12, Math.round(Math.min(width, height) / 14));
  for (let y = 0; y < height; y++) {
    const y0 = Math.max(0, y - rad);
    const y1 = Math.min(height - 1, y + rad);
    for (let x = 0; x < width; x++) {
      const x0 = Math.max(0, x - rad);
      const x1 = Math.min(width - 1, x + rad);
      const area = (y1 - y0 + 1) * (x1 - x0 + 1);
      const sum = intImg[(y1 + 1) * (width + 1) + (x1 + 1)] -
                  intImg[y0 * (width + 1) + (x1 + 1)] -
                  intImg[(y1 + 1) * (width + 1) + x0] +
                  intImg[y0 * (width + 1) + x0];
      const localMean = sum / area;
      if (gray[y * width + x] < localMean * 0.78 && gray[y * width + x] < 175) {
        mask[y * width + x] = 1;
      }
    }
  }
  const vBorders = [];
  for (let x = 0; x < width; x++) {
    let cnt = 0;
    for (let y = 0; y < height; y++) if (mask[y * width + x]) cnt++;
    if (cnt > height * 0.45) {
      vBorders.push(x);
      for (let y = 0; y < height; y++) mask[y * width + x] = 0;
    }
  }
  for (let y = 0; y < height; y++) {
    let cnt = 0;
    for (let x = 0; x < width; x++) if (mask[y * width + x]) cnt++;
    if (cnt > width * 0.45) {
      for (let x = 0; x < width; x++) mask[y * width + x] = 0;
    }
  }
  return { mask, gray, vBorders };
}

function findHandAnchorsBW(img, mask) {
  const { width, height } = img;
  const visited = new Uint8Array(width * height);
  const blobs = [];
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const p = y * width + x;
      if (!mask[p] || visited[p]) continue;
      let minX = x, maxX = x, minY = y, maxY = y, count = 0;
      const q = [p];
      visited[p] = 1;
      while (q.length) {
        const cur = q.pop();
        const cy = Math.floor(cur / width), cx = cur % width;
        count++;
        if (cx < minX) minX = cx;
        if (cx > maxX) maxX = cx;
        if (cy < minY) minY = cy;
        if (cy > maxY) maxY = cy;
        for (let dy = -1; dy <= 1; dy++) {
          for (let dx = -1; dx <= 1; dx++) {
            const nx = cx + dx, ny = cy + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
              const np = ny * width + nx;
              if (mask[np] && !visited[np]) {
                visited[np] = 1;
                q.push(np);
              }
            }
          }
        }
      }
      const bw = maxX - minX + 1, bh = maxY - minY + 1;
      if (count >= 12 && bw >= 6 && bh >= 6 && bw <= height * 0.08 && bh <= height * 0.08 &&
          bw / bh >= 0.55 && bw / bh <= 1.45) {
        blobs.push({ minX, maxX, minY, maxY, cx: (minX + maxX) / 2, cy: (minY + maxY) / 2, w: bw, h: bh });
      }
    }
  }
  const stacks = [];
  for (let i = 0; i < blobs.length; i++) {
    const b0 = blobs[i];
    if (b0.cx >= width * 0.86) continue;
    const aligned = blobs.filter(b =>
      Math.abs(b.cx - b0.cx) <= Math.max(5, b0.w * 0.45) &&
      Math.abs(b.h - b0.h) <= Math.max(5, b0.h * 0.45) &&
      b.cy >= b0.cy
    ).sort((a, b) => a.cy - b.cy);
    for (let j = 0; j < aligned.length; j++) {
      for (let k = j + 1; k < aligned.length; k++) {
        const b1 = aligned[j], b2 = aligned[k];
        const dy = b2.cy - b1.cy;
        if (dy < b0.h * 1.05 || dy > b0.h * 2.5) continue;
        if (Math.abs((b1.cy - b0.cy) - dy) > Math.max(4, dy * 0.25)) continue;
        const b3 = aligned.find(b => Math.abs((b.cy - b2.cy) - dy) <= Math.max(4, dy * 0.25));
        if (b3) {
          stacks.push({
            heart: b1,
            diamond: b2,
            cx: (b0.cx + b1.cx + b2.cx + b3.cx) / 4,
            cy: (b1.cy + b2.cy) / 2,
            dy: (b3.cy - b0.cy) / 3,
            xStart: Math.round(Math.max(b0.maxX, b1.maxX, b2.maxX, b3.maxX) + Math.max(3, b0.w * 0.22)),
            iconW: (b0.w + b1.w + b2.w + b3.w) / 4,
            iconH: (b0.h + b1.h + b2.h + b3.h) / 4
          });
        }
      }
    }
  }
  return selectBestCompassQuad(stacks, width, height);
}

function selectBestCompassQuad(pairs, width, height) {
  if (!pairs || pairs.length < 4) return null;
  let bestQuad = null, bestScore = 1e9;
  for (let iN = 0; iN < pairs.length; iN++) {
    for (let iS = 0; iS < pairs.length; iS++) {
      if (iS === iN) continue;
      const N = pairs[iN], S = pairs[iS];
      if (S.cy - N.cy < height * 0.28) continue;
      if (Math.abs(N.cx - S.cx) > width * 0.18) continue;
      for (let iW = 0; iW < pairs.length; iW++) {
        if (iW === iN || iW === iS) continue;
        for (let iE = 0; iE < pairs.length; iE++) {
          if (iE === iN || iE === iS || iE === iW) continue;
          const W = pairs[iW], E = pairs[iE];
          if (E.cx - W.cx < width * 0.32) continue;
          if (W.cx >= N.cx - width * 0.08 || E.cx <= N.cx + width * 0.08) continue;
          if (Math.abs(W.cy - E.cy) > height * 0.12) continue;
          const midY = (W.cy + E.cy) / 2;
          const midX = (W.cx + E.cx) / 2;
          if (midY <= N.cy + N.dy || midY >= S.cy - S.dy) continue;
          const score =
            Math.abs(N.cx - S.cx) * 2.5 +
            Math.abs(W.cy - E.cy) * 2.5 +
            Math.abs((N.cx + S.cx) / 2 - midX) * 1.5 +
            Math.abs((N.cy + S.cy) / 2 - midY) * 1.5;
          if (score < bestScore) {
            bestScore = score;
            bestQuad = { N, W, E, S };
          }
        }
      }
    }
  }
  return bestQuad;
}

function findHandAnchors(img, mask) {
  const { width, height, data } = img;
  const stride = data.length === width * height * 4 ? 4 : 3;
  const visited = new Uint8Array(width * height);
  const redBlobs = [];
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const p = y * width + x;
      if (visited[p]) continue;
      const r = data[p * stride], g = data[p * stride + 1], b = data[p * stride + 2];
      if (r - Math.max(g, b) > 35 && r > 90) {
        let minX = x, maxX = x, minY = y, maxY = y, count = 0;
        const q = [p];
        visited[p] = 1;
        while (q.length) {
          const cur = q.pop();
          const cy = Math.floor(cur / width), cx = cur % width;
          count++;
          if (cx < minX) minX = cx;
          if (cx > maxX) maxX = cx;
          if (cy < minY) minY = cy;
          if (cy > maxY) maxY = cy;
          for (let dy = -1; dy <= 1; dy++) {
            for (let dx = -1; dx <= 1; dx++) {
              const nx = cx + dx, ny = cy + dy;
              if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                const np = ny * width + nx;
                if (!visited[np]) {
                  const nr = data[np * stride], ng = data[np * stride + 1], nb = data[np * stride + 2];
                  if (nr - Math.max(ng, nb) > 35 && nr > 90) {
                    visited[np] = 1;
                    q.push(np);
                  }
                }
              }
            }
          }
        }
        const bw = maxX - minX + 1, bh = maxY - minY + 1;
        if (count >= 12 && bw >= 6 && bh >= 6 && bw <= height * 0.12 && bh <= height * 0.12) {
          redBlobs.push({
            minX, maxX, minY, maxY,
            cx: (minX + maxX) / 2,
            cy: (minY + maxY) / 2,
            w: bw, h: bh, count
          });
        }
      }
    }
  }

  const pairs = [];
  for (let i = 0; i < redBlobs.length; i++) {
    for (let j = 0; j < redBlobs.length; j++) {
      if (i === j) continue;
      const a = redBlobs[i], b = redBlobs[j];
      const dy = b.cy - a.cy;
      if (Math.abs(a.cx - b.cx) <= Math.max(8, a.w * 0.6) &&
          dy >= a.h * 1.05 && dy <= a.h * 2.6 &&
          a.cx < width * 0.86) {
        const spadeY = Math.round(a.cy - dy);
        const clubY = Math.round(b.cy + dy);
        if (spadeY < 2 || clubY >= height - 2) continue;
        pairs.push({
          heart: a,
          diamond: b,
          cx: (a.cx + b.cx) / 2,
          cy: (a.cy + b.cy) / 2,
          dy,
          xStart: Math.round(Math.max(a.maxX, b.maxX) + Math.max(3, a.w * 0.22)),
          iconW: (a.w + b.w) / 2,
          iconH: (a.h + b.h) / 2
        });
      }
    }
  }
  const quad = selectBestCompassQuad(pairs, width, height);
  if (quad) return quad;
  return mask ? findHandAnchorsBW(img, mask) : null;
}

function rasterizeGlyphPatch(mask, width, box) {
  const vec = new Float32Array(OCR_TW * OCR_TH);
  let norm = 0;
  for (let ty = 0; ty < OCR_TH; ty++) {
    const sy0 = box.y0 + (ty / OCR_TH) * box.h;
    const sy1 = box.y0 + ((ty + 1) / OCR_TH) * box.h;
    for (let tx = 0; tx < OCR_TW; tx++) {
      const sx0 = box.x0 + (tx / OCR_TW) * box.w;
      const sx1 = box.x0 + ((tx + 1) / OCR_TW) * box.w;
      let sum = 0, cnt = 0;
      for (let y = Math.floor(sy0); y <= Math.min(box.y1, Math.floor(sy1)); y++) {
        for (let x = Math.floor(sx0); x <= Math.min(box.x1, Math.floor(sx1)); x++) {
          sum += mask[y * width + x];
          cnt++;
        }
      }
      const val = cnt > 0 ? sum / cnt : 0;
      vec[ty * OCR_TW + tx] = val;
      norm += val * val;
    }
  }
  norm = Math.sqrt(norm) || 1;
  for (let i = 0; i < vec.length; i++) vec[i] /= norm;
  return vec;
}

function segmentSuitStrip(mask, width, height, x0, x1, yCenter, dy) {
  const y0 = Math.max(0, Math.round(yCenter - dy * 0.42));
  const y1 = Math.min(height - 1, Math.round(yCenter + dy * 0.42));
  const colDark = [];
  for (let x = x0; x < x1; x++) {
    let dark = 0;
    for (let y = y0; y <= y1; y++) {
      if (mask[y * width + x]) dark++;
    }
    colDark.push(dark);
  }
  const rawSegs = [];
  let inSeg = false, segStart = 0, blankRun = 0;
  for (let i = 0; i < colDark.length; i++) {
    if (colDark[i] > 0) {
      if (!inSeg) {
        inSeg = true;
        segStart = x0 + i;
      }
      blankRun = 0;
    } else if (inSeg) {
      inSeg = false;
      rawSegs.push([segStart, x0 + i - 1]);
    } else {
      blankRun++;
      if (rawSegs.length > 0 && blankRun > dy * 0.62) break;
    }
  }
  if (inSeg) rawSegs.push([segStart, x1 - 1]);

  const boxes = [];
  function addBox(sx0, sx1) {
    let sy0 = y1, sy1 = y0, ink = 0;
    for (let y = y0; y <= y1; y++) {
      for (let x = sx0; x <= sx1; x++) {
        if (mask[y * width + x]) {
          ink++;
          if (y < sy0) sy0 = y;
          if (y > sy1) sy1 = y;
        }
      }
    }
    if (ink < 3) return;
    const w = sx1 - sx0 + 1;
    const h = sy1 - sy0 + 1;
    if (h <= dy * 0.25 && w >= dy * 0.35) {
      boxes.push({ x0: sx0, x1: sx1, y0: sy0, y1: sy1, w, h, isVoid: true });
      return;
    }
    if (w > h * 1.18 && h >= dy * 0.5) {
      let bestSplit = -1, minCol = 1e9;
      const search0 = sx0 + Math.max(4, Math.floor(w * 0.25));
      const search1 = sx0 + Math.min(w - 5, Math.ceil(w * 0.75));
      for (let x = search0; x <= search1; x++) {
        let c = 0;
        for (let y = sy0; y <= sy1; y++) {
          if (mask[y * width + x]) c++;
        }
        if (c < minCol) {
          minCol = c;
          bestSplit = x;
        }
      }
      if (bestSplit > sx0 && minCol <= 3) {
        addBox(sx0, bestSplit - 1);
        addBox(bestSplit + 1, sx1);
        return;
      }
    }
    boxes.push({ x0: sx0, x1: sx1, y0: sy0, y1: sy1, w, h, isVoid: false });
  }
  for (const [sx0, sx1] of rawSegs) addBox(sx0, sx1);

  const clean = boxes.filter(b => b.isVoid || b.h >= dy * 0.5);
  if (clean.length === 1 && clean[0].isVoid) {
    return [];
  }
  const nonVoid = clean.filter(b => !b.isVoid);

  const tokens = [];
  for (let i = 0; i < nonVoid.length; i++) {
    const b = nonVoid[i];
    const aspect = b.w / b.h;
    if (aspect < 0.50 && i + 1 < nonVoid.length && (nonVoid[i + 1].w / nonVoid[i + 1].h) >= 0.52) {
      const next = nonVoid[i + 1];
      tokens.push({
        isTen: true,
        oneBox: b,
        zeroBox: next,
        w: next.x1 - b.x0 + 1,
        h: Math.max(b.h, next.h)
      });
      i++;
    } else {
      tokens.push({
        isTen: false,
        box: b,
        w: b.w,
        h: b.h,
        aspect,
        vec: rasterizeGlyphPatch(mask, width, b)
      });
    }
  }
  return tokens;
}

function scoreOcrToken(tok, rank) {
  if (rank === 'T') {
    return tok.isTen ? 2.5 : -4.0;
  }
  if (tok.isTen) {
    return -4.0;
  }
  const tpl = OCR_TEMPLATE_DATA.tpl[rank];
  let dot = 0;
  for (let i = 0; i < OCR_TW * OCR_TH; i++) dot += tok.vec[i] * tpl[i];
  const aspectDiff = Math.abs(tok.aspect - OCR_TEMPLATE_DATA.aspect[rank]);
  return dot * 2.0 - aspectDiff * 1.5;
}

function solveSuitPartitionDP(suitTokensByHand) {
  const dirs = ['N', 'W', 'E', 'S'];
  const lens = dirs.map(d => suitTokensByHand[d].length);
  const totalSlots = lens[0] + lens[1] + lens[2] + lens[3];

  if (totalSlots === 13) {
    const [L0, L1, L2, L3] = lens;
    const S0 = L0 + 1, S1 = L1 + 1, S2 = L2 + 1, S3 = L3 + 1;
    const numStates = S0 * S1 * S2 * S3;
    const dp = new Float32Array(numStates).fill(-1e9);
    const parent = new Int32Array(13 * numStates).fill(-1);
    dp[0] = 0;

    for (let rIdx = 0; rIdx < 13; rIdx++) {
      const rank = OCR_RANKS[rIdx];
      const nextDp = new Float32Array(numStates).fill(-1e9);
      for (let i0 = 0; i0 <= L0; i0++) {
        for (let i1 = 0; i1 <= L1; i1++) {
          for (let i2 = 0; i2 <= L2; i2++) {
            for (let i3 = 0; i3 <= L3; i3++) {
              if (i0 + i1 + i2 + i3 !== rIdx) continue;
              const st = ((i0 * S1 + i1) * S2 + i2) * S3 + i3;
              const curVal = dp[st];
              if (curVal <= -1e8) continue;
              const idxs = [i0, i1, i2, i3];
              for (let h = 0; h < 4; h++) {
                if (idxs[h] < lens[h]) {
                  const sc = curVal + scoreOcrToken(suitTokensByHand[dirs[h]][idxs[h]], rank);
                  const n0 = i0 + (h === 0 ? 1 : 0);
                  const n1 = i1 + (h === 1 ? 1 : 0);
                  const n2 = i2 + (h === 2 ? 1 : 0);
                  const n3 = i3 + (h === 3 ? 1 : 0);
                  const nst = ((n0 * S1 + n1) * S2 + n2) * S3 + n3;
                  if (sc > nextDp[nst]) {
                    nextDp[nst] = sc;
                    parent[rIdx * numStates + nst] = h;
                  }
                }
              }
            }
          }
        }
      }
      dp.set(nextDp);
    }
    const res = { N: [], W: [], E: [], S: [] };
    let c0 = L0, c1 = L1, c2 = L2, c3 = L3;
    for (let rIdx = 12; rIdx >= 0; rIdx--) {
      const st = ((c0 * S1 + c1) * S2 + c2) * S3 + c3;
      const h = parent[rIdx * numStates + st];
      if (h >= 0 && h < 4) {
        res[dirs[h]].unshift(OCR_RANKS[rIdx]);
        if (h === 0) c0--;
        else if (h === 1) c1--;
        else if (h === 2) c2--;
        else c3--;
      }
    }
    return res;
  }

  const res = {};
  for (const d of dirs) {
    const toks = suitTokensByHand[d];
    const m = toks.length;
    if (m === 0) { res[d] = []; continue; }
    const K = Math.min(m, 13);
    const dp = Array.from({ length: K }, () => new Float32Array(13).fill(-1e9));
    const par = Array.from({ length: K }, () => new Int32Array(13).fill(-1));
    for (let r = 0; r < 13; r++) dp[0][r] = scoreOcrToken(toks[0], OCR_RANKS[r]);
    for (let k = 1; k < K; k++) {
      for (let r = k; r < 13; r++) {
        const s = scoreOcrToken(toks[k], OCR_RANKS[r]);
        for (let pr = k - 1; pr < r; pr++) {
          if (dp[k - 1][pr] + s > dp[k][r]) {
            dp[k][r] = dp[k - 1][pr] + s;
            par[k][r] = pr;
          }
        }
      }
    }
    let bestR = 0;
    for (let r = 1; r < 13; r++) if (dp[K - 1][r] > dp[K - 1][bestR]) bestR = r;
    const seq = [];
    for (let k = K - 1; k >= 0; k--) {
      seq.unshift(OCR_RANKS[bestR]);
      bestR = par[k][bestR];
    }
    res[d] = seq;
  }
  return res;
}

function formatSuitRanksArrayToHandString(suitsArr) {
  const suitKeys = ['S', 'H', 'D', 'C'];
  return suitKeys.map((sk, idx) => {
    const cards = (suitsArr[idx] && suitsArr[idx].length > 0) ? suitsArr[idx].join('') : '-';
    return `${sk} ${cards};`;
  }).join(' ');
}

function recognizeBoardFromImageData(img) {
  const { mask, vBorders } = computeBinaryMask(img);
  const anchors = findHandAnchors(img, mask);
  if (!anchors) return null;

  const dirs = ['N', 'W', 'E', 'S'];
  const tokensBySuitAndHand = [{}, {}, {}, {}];
  for (const dir of dirs) {
    const h = anchors[dir];
    let xEnd = (dir === 'W') ? Math.floor(anchors.E.cx - anchors.E.iconW) : img.width - 4;
    for (const bx of vBorders) {
      if (bx > h.xStart + h.dy * 1.2 && bx < xEnd) {
        xEnd = bx - 2;
        break;
      }
    }
    const rowsY = [h.heart.cy - h.dy, h.heart.cy, h.diamond.cy, h.diamond.cy + h.dy];
    for (let sIdx = 0; sIdx < 4; sIdx++) {
      tokensBySuitAndHand[sIdx][dir] = segmentSuitStrip(
        mask, img.width, img.height, h.xStart, xEnd, rowsY[sIdx], h.dy
      );
    }
  }
  const recognized = { N: [], W: [], E: [], S: [] };
  for (let sIdx = 0; sIdx < 4; sIdx++) {
    const suitRes = solveSuitPartitionDP(tokensBySuitAndHand[sIdx]);
    for (const dir of dirs) {
      recognized[dir].push(suitRes[dir]);
    }
  }
  const hands = {
    N: formatSuitRanksArrayToHandString(recognized.N),
    E: formatSuitRanksArrayToHandString(recognized.E),
    S: formatSuitRanksArrayToHandString(recognized.S),
    W: formatSuitRanksArrayToHandString(recognized.W)
  };
  return { hands, anchors, raw: recognized };
}

function detectAllBoardsInImageData(img) {
  const { width, height, data } = img;
  const stride = data.length === width * height * 4 ? 4 : 3;
  const scale = Math.max(1, Math.floor(Math.max(width, height) / 1100));
  const sw = Math.floor(width / scale), sh = Math.floor(height / scale);
  const visited = new Uint8Array(sw * sh);
  const redBlobs = [];
  for (let y = 0; y < sh; y++) {
    for (let x = 0; x < sw; x++) {
      const p = y * sw + x;
      if (visited[p]) continue;
      const origP = (y * scale * width + x * scale) * stride;
      const r = data[origP], g = data[origP + 1], b = data[origP + 2];
      if (r - Math.max(g, b) > 35 && r > 90) {
        let minX = x, maxX = x, minY = y, maxY = y, count = 0;
        const q = [p];
        visited[p] = 1;
        while (q.length) {
          const cur = q.pop();
          const cy = Math.floor(cur / sw), cx = cur % sw;
          count++;
          if (cx < minX) minX = cx;
          if (cx > maxX) maxX = cx;
          if (cy < minY) minY = cy;
          if (cy > maxY) maxY = cy;
          for (let dy = -1; dy <= 1; dy++) {
            for (let dx = -1; dx <= 1; dx++) {
              const nx = cx + dx, ny = cy + dy;
              if (nx >= 0 && nx < sw && ny >= 0 && ny < sh) {
                const np = ny * sw + nx;
                if (!visited[np]) {
                  const op = (ny * scale * width + nx * scale) * stride;
                  const nr = data[op], ng = data[op + 1], nb = data[op + 2];
                  if (nr - Math.max(ng, nb) > 35 && nr > 90) {
                    visited[np] = 1;
                    q.push(np);
                  }
                }
              }
            }
          }
        }
        const bw = (maxX - minX + 1) * scale, bh = (maxY - minY + 1) * scale;
        if (count * scale * scale >= 12 && bw >= 5 && bh >= 5 && bw <= 90 && bh <= 90) {
          redBlobs.push({
            minX: minX * scale, maxX: maxX * scale,
            minY: minY * scale, maxY: maxY * scale,
            cx: ((minX + maxX) / 2) * scale,
            cy: ((minY + maxY) / 2) * scale,
            w: bw, h: bh
          });
        }
      }
    }
  }
  const pairs = [];
  for (let i = 0; i < redBlobs.length; i++) {
    for (let j = 0; j < redBlobs.length; j++) {
      if (i === j) continue;
      const a = redBlobs[i], b = redBlobs[j];
      const dy = b.cy - a.cy;
      if (Math.abs(a.cx - b.cx) <= Math.max(8, a.w * 0.6) &&
          dy >= a.h * 1.05 && dy <= a.h * 2.6) {
        pairs.push({
          cx: (a.cx + b.cx) / 2,
          cy: (a.cy + b.cy) / 2,
          dy,
          w: (a.w + b.w) / 2
        });
      }
    }
  }
  const quads = [];
  const used = new Set();
  pairs.sort((a, b) => a.cy - b.cy || a.cx - b.cx);
  for (let iN = 0; iN < pairs.length; iN++) {
    if (used.has(iN)) continue;
    const N = pairs[iN];
    let bestQ = null, bestSc = 1e9;
    for (let iS = 0; iS < pairs.length; iS++) {
      if (iS === iN || used.has(iS)) continue;
      const S = pairs[iS];
      const vDist = S.cy - N.cy;
      if (vDist < N.dy * 4.5 || vDist > N.dy * 12) continue;
      if (Math.abs(N.cx - S.cx) > N.dy * 2.0) continue;
      const midY = (N.cy + S.cy) / 2;
      for (let iW = 0; iW < pairs.length; iW++) {
        if (used.has(iW)) continue;
        const W = pairs[iW];
        if (Math.abs(W.cy - midY) > N.dy * 1.8) continue;
        const dxW = N.cx - W.cx;
        if (dxW < N.dy * 2.5 || dxW > N.dy * 9.0) continue;
        for (let iE = 0; iE < pairs.length; iE++) {
          if (used.has(iE)) continue;
          const E = pairs[iE];
          if (Math.abs(E.cy - W.cy) > N.dy * 1.2) continue;
          const dxE = E.cx - N.cx;
          if (dxE < N.dy * 2.5 || dxE > N.dy * 9.0) continue;
          const sc = Math.abs(N.cx - S.cx) * 2 + Math.abs(W.cy - E.cy) * 2 + Math.abs(dxW - dxE);
          if (sc < bestSc) {
            bestSc = sc;
            bestQ = { iN, iS, iW, iE, N, S, W, E };
          }
        }
      }
    }
    if (bestQ) {
      used.add(bestQ.iN); used.add(bestQ.iS); used.add(bestQ.iW); used.add(bestQ.iE);
      const { N, S, W, E } = bestQ;
      const dy = (N.dy + S.dy + W.dy + E.dy) / 4;
      const spanX = E.cx - W.cx;
      const x0 = Math.max(0, Math.round(W.cx - dy * 1.1));
      const y0 = Math.max(0, Math.round(N.cy - dy * 2.8));
      const x1 = Math.min(width, Math.round(E.cx + spanX * 0.56));
      const y1 = Math.min(height, Math.round(S.cy + dy * 3.0));
      quads.push({ x: x0, y: y0, w: x1 - x0, h: y1 - y0 });
    }
  }
  quads.sort((a, b) => (Math.abs(a.y - b.y) < 50 ? a.x - b.x : a.y - b.y));
  return quads;
}

function validateFourHandsDeck(hands) {
  const counts = { N: 0, E: 0, S: 0, W: 0 };
  const seen = new Map();
  const duplicates = [];
  ['N', 'E', 'S', 'W'].forEach(seat => {
    const suits = parseHandToSuits(hands[seat] || '');
    ['S', 'H', 'D', 'C'].forEach(sk => {
      const str = (suits[sk] || '').replace(/10/g, 'T').toUpperCase();
      for (let i = 0; i < str.length; i++) {
        const r = str.charAt(i);
        if (OCR_RANKS.includes(r)) {
          counts[seat]++;
          const key = `${sk}${r}`;
          if (seen.has(key)) {
            duplicates.push(`${key} (${seen.get(key)} & ${seat})`);
          } else {
            seen.set(key, seat);
          }
        }
      }
    });
  });
  const total = counts.N + counts.E + counts.S + counts.W;
  const isExact52 = (
    counts.N === 13 && counts.E === 13 && counts.S === 13 && counts.W === 13 &&
    seen.size === 52 && duplicates.length === 0
  );
  return { isExact52, counts, uniqueCount: seen.size, duplicates };
}

// Modal DOM Elements & Interactive Crop State
const photoScanModal = document.getElementById('photoScanModal');
const closePhotoModalBtn = document.getElementById('closePhotoModalBtn');
const cancelPhotoModalBtn = document.getElementById('cancelPhotoModalBtn');
const applyPhotoModalBtn = document.getElementById('applyPhotoModalBtn');
const cropCanvas = document.getElementById('cropCanvas');
const cropCanvasViewport = document.getElementById('cropCanvasViewport');
const pdfPageBar = document.getElementById('pdfPageBar');
const pdfPrevPageBtn = document.getElementById('pdfPrevPageBtn');
const pdfNextPageBtn = document.getElementById('pdfNextPageBtn');
const pdfPageIndicator = document.getElementById('pdfPageIndicator');
const cropRotateBtn = document.getElementById('cropRotateBtn');
const cropZoomToBoxBtn = document.getElementById('cropZoomToBoxBtn');
const cropResetViewBtn = document.getElementById('cropResetViewBtn');
const cropNewPhotoBtn = document.getElementById('cropNewPhotoBtn');
const rescanCropBtn = document.getElementById('rescanCropBtn');
const detectedBoardsBar = document.getElementById('detectedBoardsBar');
const detectedBoardsList = document.getElementById('detectedBoardsList');
const scanStatusBadge = document.getElementById('scanStatusBadge');
const scanInputN = document.getElementById('scanInputN');
const scanInputE = document.getElementById('scanInputE');
const scanInputS = document.getElementById('scanInputS');
const scanInputW = document.getElementById('scanInputW');
const modalContractHint = document.getElementById('modalContractHint');
const modalBiddingHint = document.getElementById('modalBiddingHint');
const scanBiddingInput = document.getElementById('scanBiddingInput');
const scanUndoBidBtn = document.getElementById('scanUndoBidBtn');
const scanClearBidsBtn = document.getElementById('scanClearBidsBtn');
const scanBiddingError = document.getElementById('scanBiddingError');
const scanBiddingCallsList = document.getElementById('scanBiddingCallsList');
const scanBiddingCallPicker = document.getElementById('scanBiddingCallPicker');
const modalNextLeadHint = document.getElementById('modalNextLeadHint');
const scanInitialCardsInput = document.getElementById('scanInitialCardsInput');
const scanUndoCardBtn = document.getElementById('scanUndoCardBtn');
const scanClearCardsBtn = document.getElementById('scanClearCardsBtn');
const scanInitialPlayError = document.getElementById('scanInitialPlayError');
const scanInitialPlayedList = document.getElementById('scanInitialPlayedList');
const scanInitialCardPicker = document.getElementById('scanInitialCardPicker');

let scanSourceCanvas = null; // Offscreen canvas holding current (possibly rotated) image
let detectedBoardBoxes = []; // Array of { x, y, w, h, boardLabel?, pbnHands?, pbnDealer?, pbnDeclarer?, pbnContract? } in scanSourceCanvas coords
let activeDetectedBoardIdx = -1;
let lastSelectedBoardLabel = '1';
let cropBox = { x: 0, y: 0, w: 100, h: 100 }; // In scanSourceCanvas coords
let lastDetectedAnchors = null; // In cropBox-relative coords
let cropViewZoom = 1.0;
let cropViewOffsetX = 0;
let cropViewOffsetY = 0;
let cropDragMode = null; // 'move', 'nw', 'ne', 'sw', 'se', 'n', 's', 'w', 'e', 'pan'
let cropDragStartImg = { x: 0, y: 0 };
let cropDragStartClient = { x: 0, y: 0 };
let cropDragStartBox = null;
let cropPinchDist = 0;
let cropPinchStartZoom = 1.0;

// PDF & PBN Multi-Page State
let activePdfDoc = null;
let activePdfPbnBoards = [];
let activePdfPageNum = 1;
let activePdfTotalPages = 1;
let pdfPageBoardCounts = {};
let pdfJsLoadPromise = null;

function normalizeCallToken(tok) {
  if (!tok) return null;
  const s = tok.trim()
    .replace(/♠/g, 'S')
    .replace(/♥/g, 'H')
    .replace(/♦/g, 'D')
    .replace(/♣/g, 'C')
    .toUpperCase();
  if (!s) return null;
  if (s === 'P' || s === 'PASS' || s === '-') return 'P';
  if (s === 'X' || s === 'DBL' || s === 'DOUBLE') return 'X';
  if (s === 'XX' || s === 'RDBL' || s === 'REDOUBLE') return 'XX';
  if (s.length >= 2 && s[0] >= '1' && s[0] <= '7') {
    const lvl = s[0];
    const strain = s.slice(1);
    if (strain === 'C' || strain === 'D' || strain === 'H' || strain === 'S') {
      return `${lvl}${strain}`;
    }
    if (strain === 'N' || strain === 'NT') {
      return `${lvl}NT`;
    }
  }
  return null;
}

function callStrainOrderIndex(strain) {
  if (strain === 'C') return 0;
  if (strain === 'D') return 1;
  if (strain === 'H') return 2;
  if (strain === 'S') return 3;
  if (strain === 'NT') return 4;
  return -1;
}

function formatCallDisplayHtml(callCode) {
  if (callCode === 'P') return 'Pass';
  if (callCode === 'X') return 'X';
  if (callCode === 'XX') return 'XX';
  if (callCode && callCode.length >= 2) {
    const lvl = callCode[0];
    const strain = callCode.slice(1);
    if (strain === 'S') return `${lvl}<span class="card-suit-s">♠</span>`;
    if (strain === 'H') return `${lvl}<span class="card-suit-h">♥</span>`;
    if (strain === 'D') return `${lvl}<span class="card-suit-d">♦</span>`;
    if (strain === 'C') return `${lvl}<span class="card-suit-c">♣</span>`;
    if (strain === 'NT') return `${lvl}NT`;
  }
  return escapeHtml(callCode || '');
}

function parseModalFirstRoundCalls() {
  const order = ['N', 'E', 'S', 'W'];
  const dealerIdx = Math.max(0, order.indexOf(modalDealerSeat || 'N'));
  const rawInput = scanBiddingInput ? scanBiddingInput.value.trim() : '';
  const rawTokens = rawInput ? rawInput.split(/[\s,;/]+/).filter(Boolean) : [];
  const calls = [];
  let error = null;

  if (rawTokens.length > 4) {
    error = 'First round of bidding can have at most 4 calls (one per seat starting from Dealer).';
  }

  let highestBidRank = -1;
  let lastNonPassCall = null;
  let lastNonPassSeatIdx = -1;

  const limit = Math.min(4, rawTokens.length);
  for (let i = 0; i < limit; i++) {
    const rawTok = rawTokens[i];
    const norm = normalizeCallToken(rawTok);
    const seatIdx = (dealerIdx + i) % 4;
    const seat = order[seatIdx];
    if (!norm) {
      error = `Invalid call "${rawTok}" (expected e.g. P, 1S, X, 1NT, 2H).`;
      break;
    }
    if (norm === 'X') {
      if (!lastNonPassCall || lastNonPassCall === 'X' || lastNonPassCall === 'XX') {
        error = `Cannot double (${seat}) without a preceding contract bid.`;
        break;
      }
      if ((seatIdx % 2) === (lastNonPassSeatIdx % 2)) {
        error = `${seat} cannot double partner's (${order[lastNonPassSeatIdx]}) bid.`;
        break;
      }
    } else if (norm === 'XX') {
      if (lastNonPassCall !== 'X') {
        error = `Cannot redouble (${seat}) unless the last non-pass call was X.`;
        break;
      }
      if ((seatIdx % 2) === (lastNonPassSeatIdx % 2)) {
        error = `${seat} cannot redouble partner's double.`;
        break;
      }
    } else if (norm !== 'P') {
      const lvl = parseInt(norm[0], 10);
      const strain = norm.slice(1);
      const bidRank = (lvl - 1) * 5 + callStrainOrderIndex(strain);
      if (bidRank <= highestBidRank) {
        error = `Bid ${norm} by ${seat} is not higher than the preceding bid.`;
        break;
      }
      highestBidRank = bidRank;
    }

    const afterStr = calls.map(c => c.bid).join(' ');
    calls.push({
      seat,
      seatIdx,
      bid: norm,
      after: afterStr
    });

    if (norm !== 'P') {
      lastNonPassCall = norm;
      lastNonPassSeatIdx = seatIdx;
    }
  }

  const nextTurnIdx = calls.length < 4 ? (dealerIdx + calls.length) % 4 : -1;
  const nextTurnSeat = nextTurnIdx >= 0 ? order[nextTurnIdx] : null;
  return {
    dealerSeat: order[dealerIdx],
    dealerIdx,
    calls,
    nextTurnIdx,
    nextTurnSeat,
    highestBidRank,
    lastNonPassCall,
    lastNonPassSeatIdx,
    error
  };
}

function appendModalCallToken(callCode) {
  if (!scanBiddingInput) return;
  const cur = scanBiddingInput.value.trim();
  const toks = cur ? cur.split(/[\s,;/]+/).filter(Boolean) : [];
  if (toks.length >= 4) return;
  toks.push(callCode);
  scanBiddingInput.value = toks.join(' ');
  updateModalBiddingUI();
}

function updateModalBiddingUI() {
  document.querySelectorAll('.modal-dealer-btn').forEach(btn => {
    const btnDealer = btn.dataset.dealer || btn.dataset.seat;
    btn.classList.toggle('active', btnDealer === modalDealerSeat);
  });

  const parsed = parseModalFirstRoundCalls();
  const order = ['N', 'E', 'S', 'W'];
  const seatNames = { N: 'North', E: 'East', S: 'South', W: 'West' };
  const declIdx = Math.max(0, order.indexOf(currentDeclarerSeat || 'N'));
  const lhoSeat = order[(declIdx + 1) % 4];
  const rhoSeat = order[(declIdx + 3) % 4];
  const getRoleLabel = (origIdx) => {
    if (origIdx === declIdx) return 'Declarer';
    if (origIdx === (declIdx + 1) % 4) return 'LHO';
    if (origIdx === (declIdx + 2) % 4) return 'Dummy';
    return 'RHO';
  };

  if (modalBiddingHint) {
    if (parsed.calls.length === 0) {
      modalBiddingHint.textContent =
        `Dealer: ${seatNames[parsed.dealerSeat]} (${getRoleLabel(parsed.dealerIdx)})`;
    } else if (parsed.calls.length < 4 && parsed.nextTurnSeat) {
      modalBiddingHint.textContent =
        `${parsed.calls.length}/4 calls · Next: ${seatNames[parsed.nextTurnSeat]} (${getRoleLabel(parsed.nextTurnIdx)})`;
    } else {
      modalBiddingHint.textContent = 'First round complete (4/4 calls)';
    }
  }

  const hasInputText = Boolean(scanBiddingInput && scanBiddingInput.value.trim().length > 0);
  if (scanUndoBidBtn) scanUndoBidBtn.disabled = !hasInputText;
  if (scanClearBidsBtn) scanClearBidsBtn.disabled = !hasInputText;

  if (scanBiddingError) {
    if (parsed.error) {
      scanBiddingError.textContent = parsed.error;
      scanBiddingError.classList.remove('hidden');
    } else {
      scanBiddingError.textContent = '';
      scanBiddingError.classList.add('hidden');
    }
  }

  if (scanBiddingCallsList) {
    scanBiddingCallsList.innerHTML = '';
    if (parsed.calls.length === 0) {
      scanBiddingCallsList.classList.add('hidden');
    } else {
      scanBiddingCallsList.classList.remove('hidden');
      parsed.calls.forEach(c => {
        const isDefender = (c.seat === lhoSeat || c.seat === rhoSeat);
        const roleTag = getRoleLabel(c.seatIdx);
        const pill = document.createElement('span');
        pill.className = `scan-bidding-call-pill${isDefender ? ' defender-call' : ''}`;
        pill.innerHTML =
          `<strong>${c.seat} (${roleTag}):</strong> ${formatCallDisplayHtml(c.bid)}` +
          (isDefender ? ' <span class="trick-winner-tag">[constrained]</span>' : '');
        scanBiddingCallsList.appendChild(pill);
      });
    }
  }

  if (scanBiddingCallPicker) {
    scanBiddingCallPicker.innerHTML = '';
    if (parsed.calls.length >= 4 || parsed.error) {
      return;
    }
    const nextSeat = parsed.nextTurnSeat || parsed.dealerSeat;
    const nextRole = getRoleLabel(parsed.nextTurnIdx >= 0 ? parsed.nextTurnIdx : parsed.dealerIdx);
    const header = document.createElement('div');
    header.className = 'scan-picker-header';
    header.innerHTML =
      `Quick add call for <strong>${seatNames[nextSeat]} (${nextRole})</strong>:`;
    scanBiddingCallPicker.appendChild(header);

    const quickCalls = ['P'];
    if (parsed.lastNonPassCall && parsed.lastNonPassCall !== 'X' && parsed.lastNonPassCall !== 'XX' &&
        (parsed.nextTurnIdx % 2) !== (parsed.lastNonPassSeatIdx % 2)) {
      quickCalls.push('X');
    }
    const candidateBids = [
      '1C', '1D', '1H', '1S', '1NT',
      '2C', '2D', '2H', '2S', '2NT',
      '3C', '3D', '3H', '3S', '3NT'
    ];
    candidateBids.forEach(b => {
      const lvl = parseInt(b[0], 10);
      const strain = b.slice(1);
      const rank = (lvl - 1) * 5 + callStrainOrderIndex(strain);
      if (rank > parsed.highestBidRank) {
        quickCalls.push(b);
      }
    });

    quickCalls.forEach(code => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'scan-bidding-call-btn';
      btn.innerHTML = formatCallDisplayHtml(code);
      btn.title = `Add ${code} by ${seatNames[nextSeat]}`;
      btn.addEventListener('click', () => {
        appendModalCallToken(code);
      });
      scanBiddingCallPicker.appendChild(btn);
    });
  }
}

function parseModalSingleCardToken(tok) {
  if (!tok) return null;
  const s = tok.trim()
    .replace(/♠/g, 'S')
    .replace(/♥/g, 'H')
    .replace(/♦/g, 'D')
    .replace(/♣/g, 'C')
    .toUpperCase();
  if (!s) return null;
  let suit = null;
  let rankPart = '';
  if ('SHDC'.includes(s[0])) {
    suit = s[0];
    rankPart = s.slice(1);
  } else if ('SHDC'.includes(s[s.length - 1])) {
    suit = s[s.length - 1];
    rankPart = s.slice(0, -1);
  } else {
    return null;
  }
  if (rankPart === '10') rankPart = 'T';
  if (rankPart.length !== 1 || !'AKQJT98765432'.includes(rankPart)) {
    return null;
  }
  return { suit, rank: rankPart, code: `${suit}${rankPart}` };
}

function cloneModalHandsMap(handsMap) {
  const copy = {};
  ['N', 'E', 'S', 'W'].forEach(seat => {
    copy[seat] = {
      S: (handsMap[seat] && handsMap[seat].S) ? handsMap[seat].S.slice() : [],
      H: (handsMap[seat] && handsMap[seat].H) ? handsMap[seat].H.slice() : [],
      D: (handsMap[seat] && handsMap[seat].D) ? handsMap[seat].D.slice() : [],
      C: (handsMap[seat] && handsMap[seat].C) ? handsMap[seat].C.slice() : []
    };
  });
  return copy;
}

function strainToTrumpCode(strain) {
  if (strain === 'SPADES') return 'S';
  if (strain === 'HEARTS') return 'H';
  if (strain === 'DIAMONDS') return 'D';
  if (strain === 'CLUBS') return 'C';
  return 'NT';
}

function simulateModalInitialPlay(handsObj, declarerSeat, contractStrain) {
  const order = ['N', 'E', 'S', 'W'];
  const declIdx = Math.max(0, order.indexOf(declarerSeat || currentDeclarerSeat || 'N'));
  const trumpCode = strainToTrumpCode(contractStrain || modalContractStrain);

  const srcHands = handsObj || {
    N: scanInputN ? scanInputN.value : '',
    E: scanInputE ? scanInputE.value : '',
    S: scanInputS ? scanInputS.value : '',
    W: scanInputW ? scanInputW.value : ''
  };

  const initialHandsMap = {};
  order.forEach(seat => {
    const suits = parseHandToSuits(srcHands[seat] || '');
    initialHandsMap[seat] = {};
    ['S', 'H', 'D', 'C'].forEach(s => {
      initialHandsMap[seat][s] = tokenizeSuitRanks(suits[s] || '-')
        .map(t => (t === '10' ? 'T' : t.toUpperCase()))
        .filter(t => 'AKQJT98765432'.includes(t));
    });
  });

  const remHands = cloneModalHandsMap(initialHandsMap);
  let remHandsAfterCompleted = cloneModalHandsMap(initialHandsMap);

  let currentLeaderOrigIdx = (declIdx + 1) % 4; // LHO leads to Trick 1
  const completedTricks = [];
  let currentTrickCards = [];
  let nsCompletedTricksWon = 0;
  const voidSuits = { E: new Set(), W: new Set() };
  const playedCodes = new Set();
  let error = null;

  const rawInput = scanInitialCardsInput ? scanInitialCardsInput.value.trim() : '';
  const rawTokens = rawInput ? rawInput.split(/[\s,;/]+/).filter(Boolean) : [];

  for (let i = 0; i < rawTokens.length; i++) {
    const rawTok = rawTokens[i];
    const turnOffset = currentTrickCards.length;
    const seatOrigIdx = (currentLeaderOrigIdx + turnOffset) % 4;
    const seatKey = order[seatOrigIdx];
    const rotatedIdx = (seatOrigIdx - declIdx + 4) % 4;

    const parsed = parseModalSingleCardToken(rawTok);
    if (!parsed) {
      error = `Invalid card "${rawTok}" (expected e.g. DQ, H10, SA).`;
      break;
    }

    if (playedCodes.has(parsed.code)) {
      error = `${parsed.code} has already been played.`;
      break;
    }

    const seatSuitArr = remHands[seatKey][parsed.suit] || [];
    const rankIdx = seatSuitArr.indexOf(parsed.rank);
    if (rankIdx === -1) {
      const holder = order.find(other =>
        (remHands[other][parsed.suit] || []).includes(parsed.rank)
      );
      if (holder) {
        error = `It is ${seatKey}'s turn, but ${parsed.code} is held by ${holder}.`;
      } else {
        error = `${seatKey} does not hold ${parsed.code}.`;
      }
      break;
    }

    if (turnOffset > 0) {
      const leadSuit = currentTrickCards[0].suit;
      if (parsed.suit !== leadSuit) {
        if ((remHands[seatKey][leadSuit] || []).length > 0) {
          error = `${seatKey} must follow suit to ${leadSuit} before playing ${parsed.code}.`;
          break;
        }
        if (rotatedIdx === 1) voidSuits.E.add(leadSuit);
        if (rotatedIdx === 3) voidSuits.W.add(leadSuit);
      }
    }

    seatSuitArr.splice(rankIdx, 1);
    playedCodes.add(parsed.code);

    const leadSuitForCard = currentTrickCards.length > 0 ? currentTrickCards[0].suit : parsed.suit;
    const cardObj = parseSingleCardToken(parsed.code, leadSuitForCard, trumpCode);
    cardObj.code = parsed.code;
    cardObj.origSeatIdx = seatOrigIdx;
    cardObj.origSeat = seatKey;
    cardObj.rotatedSeatIdx = rotatedIdx;
    cardObj.rotatedSeat = order[rotatedIdx];
    currentTrickCards.push(cardObj);

    if (currentTrickCards.length === 4) {
      const { winningIdx, winnerCard } = evaluateTrickWinner(currentTrickCards, trumpCode);
      const winnerOrigIdx = winnerCard ? winnerCard.origSeatIdx : currentLeaderOrigIdx;
      const winnerRotatedIdx = (winnerOrigIdx - declIdx + 4) % 4;
      if (winnerRotatedIdx === 0 || winnerRotatedIdx === 2) {
        nsCompletedTricksWon++;
      }
      completedTricks.push({
        trickNum: completedTricks.length + 1,
        leaderOrigIdx: currentLeaderOrigIdx,
        leaderSeat: order[currentLeaderOrigIdx],
        cards: currentTrickCards.slice(),
        winningIdx,
        winnerOrigIdx,
        winnerSeat: order[winnerOrigIdx],
        winnerRotatedIdx,
        winnerRotatedSeat: order[winnerRotatedIdx]
      });
      remHandsAfterCompleted = cloneModalHandsMap(remHands);
      currentLeaderOrigIdx = winnerOrigIdx;
      currentTrickCards = [];
    }
  }

  const nextTrickNum = completedTricks.length + 1;
  const nextSeatOrigIdx = (currentLeaderOrigIdx + currentTrickCards.length) % 4;
  const nextSeatKey = order[nextSeatOrigIdx];

  return {
    completedTricks,
    currentTrickCards,
    currentLeaderOrigIdx,
    nextTrickNum,
    nextSeatOrigIdx,
    nextSeatKey,
    nsCompletedTricksWon,
    voidSuits,
    remHandsAfterCompleted,
    remHandsAfterAll: remHands,
    error
  };
}

function appendModalCardToken(cardCode) {
  if (!scanInitialCardsInput) return;
  const cur = scanInitialCardsInput.value.trim();
  scanInitialCardsInput.value = cur ? `${cur} ${cardCode}` : cardCode;
  updateModalContractAndPlayUI();
}

function updateModalContractAndPlayUI() {
  document.querySelectorAll('.modal-contract-level-btn').forEach(btn => {
    btn.classList.toggle('active', parseInt(btn.dataset.level, 10) === modalContractLevel);
  });
  document.querySelectorAll('.modal-contract-strain-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.strain === modalContractStrain);
  });

  const sim = simulateModalInitialPlay();
  const strainSym = getSuitSymbol(modalContractStrain);
  const totalTarget = 6 + modalContractLevel;
  const remTarget = Math.max(1, totalTarget - sim.nsCompletedTricksWon);

  if (modalContractHint) {
    if (sim.nsCompletedTricksWon > 0) {
      modalContractHint.textContent =
        `${modalContractLevel}${strainSym} (${remTarget} more trick${remTarget !== 1 ? 's' : ''} needed; ${sim.nsCompletedTricksWon} won)`;
    } else {
      modalContractHint.textContent = `${modalContractLevel}${strainSym} (${totalTarget} tricks)`;
    }
  }

  const order = ['N', 'E', 'S', 'W'];
  const seatNames = { N: 'North', E: 'East', S: 'South', W: 'West' };
  const declIdx = Math.max(0, order.indexOf(currentDeclarerSeat || 'N'));
  const getRoleLabel = (origIdx) => {
    if (origIdx === declIdx) return 'Declarer';
    if (origIdx === (declIdx + 1) % 4) return 'LHO';
    if (origIdx === (declIdx + 2) % 4) return 'Dummy';
    return 'RHO';
  };

  if (modalNextLeadHint) {
    const nextRole = getRoleLabel(sim.nextSeatOrigIdx);
    const nextName = seatNames[sim.nextSeatKey] || sim.nextSeatKey;
    if (sim.completedTricks.length === 0 && sim.currentTrickCards.length === 0) {
      modalNextLeadHint.textContent = `Trick 1 opening lead by ${nextName} (${nextRole})`;
    } else if (sim.currentTrickCards.length === 0) {
      modalNextLeadHint.textContent = `Trick ${sim.nextTrickNum} lead by ${nextName} (${nextRole})`;
    } else {
      modalNextLeadHint.textContent =
        `Trick ${sim.nextTrickNum} (${sim.currentTrickCards.length}/4): ${nextName} (${nextRole}) to play`;
    }
  }

  const hasInputText = Boolean(scanInitialCardsInput && scanInitialCardsInput.value.trim().length > 0);
  if (scanUndoCardBtn) scanUndoCardBtn.disabled = !hasInputText;
  if (scanClearCardsBtn) scanClearCardsBtn.disabled = !hasInputText;

  if (scanInitialPlayError) {
    if (sim.error) {
      scanInitialPlayError.textContent = sim.error;
      scanInitialPlayError.classList.remove('hidden');
    } else {
      scanInitialPlayError.textContent = '';
      scanInitialPlayError.classList.add('hidden');
    }
  }

  if (scanInitialPlayedList) {
    scanInitialPlayedList.innerHTML = '';
    if (sim.completedTricks.length === 0 && sim.currentTrickCards.length === 0) {
      scanInitialPlayedList.classList.add('hidden');
    } else {
      scanInitialPlayedList.classList.remove('hidden');
      const formatCardsHtml = (cards) => cards.map(c =>
        `<span class="${c.suitClass}">${c.suitSym}</span>${String(c.rankStr || '').replace(/</g, '&lt;')}`
      ).join(' ');

      sim.completedTricks.forEach(tr => {
        const pill = document.createElement('span');
        pill.className = 'scan-played-trick-pill';
        pill.innerHTML =
          `<strong>T${tr.trickNum} (${tr.leaderSeat}):</strong> ${formatCardsHtml(tr.cards)} ` +
          `<span class="trick-winner-tag">→ ${tr.winnerSeat} wins</span>`;
        scanInitialPlayedList.appendChild(pill);
      });

      if (sim.currentTrickCards.length > 0) {
        const pill = document.createElement('span');
        pill.className = 'scan-played-trick-pill partial-trick';
        const leadSeat = order[sim.currentLeaderOrigIdx];
        pill.innerHTML =
          `<strong>T${sim.nextTrickNum} (${leadSeat}):</strong> ${formatCardsHtml(sim.currentTrickCards)} ` +
          `<span class="trick-winner-tag">(${sim.currentTrickCards.length}/4)</span>`;
        scanInitialPlayedList.appendChild(pill);
      }
    }
  }

  if (scanInitialCardPicker) {
    scanInitialCardPicker.innerHTML = '';
    if (sim.completedTricks.length >= 13) {
      scanInitialCardPicker.textContent = 'All 13 tricks completed.';
      return;
    }
    const nextSeatSuits = sim.remHandsAfterAll[sim.nextSeatKey] || { S: [], H: [], D: [], C: [] };
    const totalRem = ['S', 'H', 'D', 'C'].reduce((acc, s) => acc + (nextSeatSuits[s] ? nextSeatSuits[s].length : 0), 0);
    if (totalRem === 0) {
      return;
    }

    let legalSuits = new Set(['S', 'H', 'D', 'C']);
    if (sim.currentTrickCards.length > 0) {
      const leadSuit = sim.currentTrickCards[0].suit;
      if (nextSeatSuits[leadSuit] && nextSeatSuits[leadSuit].length > 0) {
        legalSuits = new Set([leadSuit]);
      }
    }

    const nextRole = getRoleLabel(sim.nextSeatOrigIdx);
    const header = document.createElement('div');
    header.className = 'scan-picker-header';
    header.innerHTML =
      `Tap a card from <strong>${seatNames[sim.nextSeatKey]} (${nextRole})</strong> to add to initial play:`;
    scanInitialCardPicker.appendChild(header);

    const suitsRow = document.createElement('div');
    suitsRow.className = 'scan-picker-suits';
    const suitDefs = [
      { key: 'S', sym: '♠', cls: 'card-suit-s' },
      { key: 'H', sym: '♥', cls: 'card-suit-h' },
      { key: 'D', sym: '♦', cls: 'card-suit-d' },
      { key: 'C', sym: '♣', cls: 'card-suit-c' }
    ];

    suitDefs.forEach(sd => {
      const group = document.createElement('div');
      group.className = 'scan-picker-suit-group';
      const symSpan = document.createElement('span');
      symSpan.className = `scan-picker-suit-sym ${sd.cls}`;
      symSpan.textContent = sd.sym;
      group.appendChild(symSpan);

      const ranks = nextSeatSuits[sd.key] || [];
      if (ranks.length === 0) {
        const emptySpan = document.createElement('span');
        emptySpan.style.color = '#64748b';
        emptySpan.style.fontSize = '0.72rem';
        emptySpan.textContent = '-';
        group.appendChild(emptySpan);
      } else {
        const isSuitLegal = legalSuits.has(sd.key) && !sim.error;
        ranks.forEach(r => {
          const btn = document.createElement('button');
          btn.type = 'button';
          btn.className = 'scan-picker-card-btn';
          btn.textContent = r;
          btn.disabled = !isSuitLegal;
          btn.title = isSuitLegal
            ? `Play ${sd.sym}${r} from ${seatNames[sim.nextSeatKey]}`
            : `Must follow suit to ${sim.currentTrickCards[0] ? sim.currentTrickCards[0].suit : ''}`;
          btn.addEventListener('click', () => {
            appendModalCardToken(`${sd.key}${r}`);
          });
          group.appendChild(btn);
        });
      }
      suitsRow.appendChild(group);
    });
    scanInitialCardPicker.appendChild(suitsRow);
  }
}

function syncModalContractFromMainUI() {
  if (trumpSelect && trumpSelect.value) {
    modalContractStrain = trumpSelect.value;
  }
  if (targetTricks && targetTricks.value) {
    const tgt = parseInt(targetTricks.value, 10);
    if (!isNaN(tgt)) {
      modalContractLevel = Math.max(1, Math.min(7, tgt - 6));
    }
  }
}

function applyBoardMetadataToModal(box) {
  if (!box) return;
  if (box.pbnDealer && ['N', 'E', 'S', 'W'].includes(box.pbnDealer)) {
    modalDealerSeat = box.pbnDealer;
  } else if (box.boardLabel) {
    const bNum = parseInt(box.boardLabel, 10);
    if (!isNaN(bNum) && bNum >= 1) {
      modalDealerSeat = ['N', 'E', 'S', 'W'][(bNum - 1) % 4];
    }
  }
  if (box.pbnDeclarer && ['N', 'E', 'S', 'W'].includes(box.pbnDeclarer)) {
    currentDeclarerSeat = box.pbnDeclarer;
  }
  if (box.pbnContract) {
    if (box.pbnContract.level >= 1 && box.pbnContract.level <= 7) {
      modalContractLevel = box.pbnContract.level;
    }
    if (box.pbnContract.strain) {
      modalContractStrain = box.pbnContract.strain;
    }
  }
  updateDeclarerBarUI(currentDeclarerSeat);
}

function parsePbnDealString(dealStr) {
  const m = (dealStr || '').trim().match(/^([NESW]):\s*(.+)$/i);
  if (!m) return null;
  const firstSeat = m[1].toUpperCase();
  const handTokens = m[2].trim().split(/\s+/);
  if (handTokens.length < 4) return null;
  const order = ['N', 'E', 'S', 'W'];
  const startIdx = order.indexOf(firstSeat);
  if (startIdx === -1) return null;
  const suitKeys = ['S', 'H', 'D', 'C'];
  const hands = {};
  for (let i = 0; i < 4; i++) {
    const seat = order[(startIdx + i) % 4];
    const suits = handTokens[i].split('.');
    if (suits.length !== 4) return null;
    hands[seat] = suitKeys.map((sk, sIdx) => {
      const r = (suits[sIdx] || '').replace(/10/g, 'T').replace(/-/g, '').toUpperCase();
      return `${sk} ${r.length > 0 ? r : '-'};`;
    }).join(' ');
  }
  return hands;
}

function parsePbnBoardsFromText(text) {
  if (!text || !text.includes('[Deal ')) return [];
  const boards = [];
  const lines = text.split(/\r?\n/);
  let curBoard = null;
  let curDealer = null;
  let curDeclarer = null;
  let curContract = null;
  for (const line of lines) {
    const bm = line.match(/^\[Board\s+"([^"]+)"\]/i);
    if (bm) curBoard = bm[1].trim();
    const dlm = line.match(/^\[Dealer\s+"([NESW])"\]/i);
    if (dlm) {
      curDealer = dlm[1].toUpperCase();
      if (boards.length > 0 && !curBoard && !boards[boards.length - 1].dealer) {
        boards[boards.length - 1].dealer = curDealer;
      }
    }
    const decm = line.match(/^\[Declarer\s+"([NESW])"\]/i);
    if (decm) {
      curDeclarer = decm[1].toUpperCase();
      if (boards.length > 0 && !curBoard && !boards[boards.length - 1].declarer) {
        boards[boards.length - 1].declarer = curDeclarer;
      }
    }
    const cm = line.match(/^\[Contract\s+"([1-7])(NT|N|S|H|D|C)/i);
    if (cm) {
      const lvl = parseInt(cm[1], 10);
      const stRaw = cm[2].toUpperCase();
      const strainMap = {
        C: 'CLUBS', D: 'DIAMONDS', H: 'HEARTS', S: 'SPADES', N: 'NOTRUMP', NT: 'NOTRUMP'
      };
      curContract = { level: lvl, strain: strainMap[stRaw] || 'NOTRUMP' };
      if (boards.length > 0 && !curBoard && !boards[boards.length - 1].contract) {
        boards[boards.length - 1].contract = curContract;
      }
    }
    const dm = line.match(/^\[Deal\s+"([^"]+)"\]/i);
    if (dm) {
      const hands = parsePbnDealString(dm[1]);
      if (hands) {
        const dealSeatMatch = dm[1].trim().match(/^([NESW]):/i);
        const fallbackDealer = dealSeatMatch ? dealSeatMatch[1].toUpperCase() : null;
        boards.push({
          board: curBoard || String(boards.length + 1),
          hands,
          dealer: curDealer || fallbackDealer,
          declarer: curDeclarer,
          contract: curContract
        });
      }
      curBoard = null;
      curDealer = null;
      curDeclarer = null;
      curContract = null;
    }
  }
  return boards;
}

async function tryDecompressPdfStream(chunk, format) {
  if (typeof DecompressionStream === 'undefined') return null;
  try {
    const ds = new DecompressionStream(format);
    const writer = ds.writable.getWriter();
    const writePromise = writer.write(chunk).then(() => writer.close()).catch(() => {});
    const reader = ds.readable.getReader();
    const out = [];
    let total = 0;
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      out.push(value);
      total += value.byteLength;
      if (total > 2 * 1024 * 1024) break;
    }
    await writePromise;
    const merged = new Uint8Array(total);
    let offset = 0;
    for (const part of out) {
      merged.set(part, offset);
      offset += part.byteLength;
    }
    return new TextDecoder('latin1').decode(merged);
  } catch (_) {
    return null;
  }
}

async function extractPbnBoardsFromPdfBuffer(arrayBuffer) {
  const bytes = new Uint8Array(arrayBuffer);
  const raw = new TextDecoder('latin1').decode(bytes);
  const direct = parsePbnBoardsFromText(raw);
  if (direct.length > 0) return direct;

  let pos = 0;
  while (true) {
    const sIdx = raw.indexOf('stream', pos);
    if (sIdx === -1) break;
    let dataStart = sIdx + 6;
    if (raw[dataStart] === '\r' && raw[dataStart + 1] === '\n') dataStart += 2;
    else if (raw[dataStart] === '\n' || raw[dataStart] === '\r') dataStart += 1;
    else { pos = sIdx + 6; continue; }

    const endIdx = raw.indexOf('endstream', dataStart);
    if (endIdx === -1) break;
    let dataEnd = endIdx;
    if (raw[dataEnd - 2] === '\r' && raw[dataEnd - 1] === '\n') dataEnd -= 2;
    else if (raw[dataEnd - 1] === '\n' || raw[dataEnd - 1] === '\r') dataEnd -= 1;

    const len = dataEnd - dataStart;
    if (len > 16 && len < 1024 * 1024) {
      const chunk = bytes.subarray(dataStart, dataEnd);
      let inflated = await tryDecompressPdfStream(chunk, 'deflate');
      if (!inflated) inflated = await tryDecompressPdfStream(chunk, 'deflate-raw');
      if (inflated && inflated.includes('[Deal ')) {
        const boards = parsePbnBoardsFromText(inflated);
        if (boards.length > 0) return boards;
      }
    }
    pos = endIdx + 9;
  }
  return [];
}

function loadPdfJs() {
  if (window.pdfjsLib) return Promise.resolve(window.pdfjsLib);
  if (pdfJsLoadPromise) return pdfJsLoadPromise;
  pdfJsLoadPromise = new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = 'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.min.js';
    script.async = true;
    script.onload = () => {
      if (window.pdfjsLib) {
        window.pdfjsLib.GlobalWorkerOptions.workerSrc =
          'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.worker.min.js';
        resolve(window.pdfjsLib);
      } else {
        pdfJsLoadPromise = null;
        reject(new Error('pdfjsLib unavailable'));
      }
    };
    script.onerror = () => {
      pdfJsLoadPromise = null;
      reject(new Error('Failed to load pdf.js'));
    };
    document.head.appendChild(script);
  });
  return pdfJsLoadPromise;
}

function renderPbnBoardsSheetToCanvas(boards, pageNum) {
  const perPage = 16;
  const startIdx = (pageNum - 1) * perPage;
  const pageBoards = boards.slice(startIdx, startIdx + perPage);
  const cols = Math.min(4, Math.max(1, pageBoards.length));
  const rows = Math.max(1, Math.ceil(pageBoards.length / cols));
  const cellW = 420;
  const cellH = 260;
  const pad = 20;

  const off = document.createElement('canvas');
  off.width = cols * cellW + pad * 2;
  off.height = rows * cellH + pad * 2;
  const ctx = off.getContext('2d');
  ctx.fillStyle = '#ffffff';
  ctx.fillRect(0, 0, off.width, off.height);

  const boxes = [];
  const suitMeta = [
    { key: 'S', sym: '♠', color: '#0f172a' },
    { key: 'H', sym: '♥', color: '#dc2626' },
    { key: 'D', sym: '♦', color: '#dc2626' },
    { key: 'C', sym: '♣', color: '#0f172a' }
  ];

  pageBoards.forEach((b, i) => {
    const col = i % cols;
    const row = Math.floor(i / cols);
    const bx = pad + col * cellW;
    const by = pad + row * cellH;

    ctx.strokeStyle = '#cbd5e1';
    ctx.lineWidth = 1.5;
    ctx.strokeRect(bx + 4, by + 4, cellW - 8, cellH - 8);

    ctx.fillStyle = '#0f172a';
    ctx.font = 'bold 14px sans-serif';
    ctx.fillText(`Board ${b.board}`, bx + 14, by + 24);

    const drawHandAt = (handStr, hx, hy) => {
      const suits = parseHandToSuits(handStr || '');
      suitMeta.forEach((sm, sIdx) => {
        const y = hy + sIdx * 17;
        ctx.fillStyle = sm.color;
        ctx.font = 'bold 14px sans-serif';
        ctx.fillText(sm.sym, hx, y);
        ctx.fillStyle = '#0f172a';
        ctx.font = '13.5px sans-serif';
        const ranks = (suits[sm.key] || '-').replace(/T/g, '10');
        ctx.fillText(ranks, hx + 16, y);
      });
    };

    drawHandAt(b.hands.N, bx + 150, by + 26);
    drawHandAt(b.hands.W, bx + 16, by + 104);
    drawHandAt(b.hands.E, bx + 268, by + 104);
    drawHandAt(b.hands.S, bx + 150, by + 184);

    boxes.push({
      x: bx + 4,
      y: by + 4,
      w: cellW - 8,
      h: cellH - 8,
      boardLabel: String(b.board),
      pbnHands: b.hands,
      pbnDealer: b.dealer || null,
      pbnDeclarer: b.declarer || null,
      pbnContract: b.contract || null
    });
  });

  return { canvas: off, boxes };
}

function updatePdfPageBarUI() {
  if (!pdfPageBar) return;
  if (activePdfTotalPages > 1) {
    pdfPageBar.classList.remove('hidden');
    if (pdfPageIndicator) {
      pdfPageIndicator.textContent = `Page ${activePdfPageNum} / ${activePdfTotalPages}`;
    }
    if (pdfPrevPageBtn) pdfPrevPageBtn.disabled = (activePdfPageNum <= 1);
    if (pdfNextPageBtn) pdfNextPageBtn.disabled = (activePdfPageNum >= activePdfTotalPages);
  } else {
    pdfPageBar.classList.add('hidden');
  }
}

async function renderActivePdfPage() {
  updatePdfPageBarUI();
  if (scanBiddingInput) scanBiddingInput.value = '';
  if (scanInitialCardsInput) scanInitialCardsInput.value = '';
  let pageBoardOffset = 0;
  for (let p = 1; p < activePdfPageNum; p++) {
    pageBoardOffset += (pdfPageBoardCounts[p] || 16);
  }

  if (activePdfDoc) {
    if (scanStatusBadge) {
      scanStatusBadge.className = 'scan-status-badge scan-status-pending';
      scanStatusBadge.textContent = `Rendering PDF page ${activePdfPageNum} of ${activePdfTotalPages}...`;
    }
    const page = await activePdfDoc.getPage(activePdfPageNum);
    const viewport = page.getViewport({ scale: 2.2 });
    const off = document.createElement('canvas');
    off.width = Math.round(viewport.width);
    off.height = Math.round(viewport.height);
    const ctx = off.getContext('2d');
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, off.width, off.height);
    await page.render({ canvasContext: ctx, viewport }).promise;
    scanSourceCanvas = off;
    runFullPhotoBoardDetection(pageBoardOffset);
    pdfPageBoardCounts[activePdfPageNum] = detectedBoardBoxes.length || 16;
    return;
  }

  if (activePdfPbnBoards.length > 0) {
    const { canvas, boxes } = renderPbnBoardsSheetToCanvas(activePdfPbnBoards, activePdfPageNum);
    scanSourceCanvas = canvas;
    detectedBoardBoxes = boxes;
    pdfPageBoardCounts[activePdfPageNum] = boxes.length;
    if (detectedBoardBoxes.length > 0) {
      activeDetectedBoardIdx = 0;
      cropBox = { ...detectedBoardBoxes[0] };
      applyBoardMetadataToModal(detectedBoardBoxes[0]);
    }
    renderDetectedBoardsPills();
    fitCropPhotoView();
    if (detectedBoardBoxes.length > 1) {
      zoomCropViewToBox();
    }
    scanCurrentCropRegion();
  }
}

function openPhotoScanModalWithPbnBoards(pbnBoards) {
  activePdfDoc = null;
  activePdfPbnBoards = pbnBoards;
  activePdfPageNum = 1;
  activePdfTotalPages = Math.max(1, Math.ceil(pbnBoards.length / 16));
  pdfPageBoardCounts = {};
  syncModalContractFromMainUI();
  if (scanBiddingInput) scanBiddingInput.value = '';
  if (scanInitialCardsInput) scanInitialCardsInput.value = '';
  if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
  if (photoScanModal) photoScanModal.classList.remove('hidden');
  updateDeclarerBarUI(currentDeclarerSeat);
  requestAnimationFrame(() => {
    renderActivePdfPage();
  });
}

async function openPhotoScanModalWithPdfFile(file) {
  activePdfDoc = null;
  activePdfPbnBoards = [];
  activePdfPageNum = 1;
  activePdfTotalPages = 1;
  pdfPageBoardCounts = {};
  syncModalContractFromMainUI();
  if (scanBiddingInput) scanBiddingInput.value = '';
  if (scanInitialCardsInput) scanInitialCardsInput.value = '';

  if (photoScanModal) photoScanModal.classList.remove('hidden');
  if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
  updateDeclarerBarUI(currentDeclarerSeat);
  if (scanStatusBadge) {
    scanStatusBadge.className = 'scan-status-badge scan-status-pending';
    scanStatusBadge.textContent = 'Loading PDF hand record...';
  }

  const pdfStartMs = performance.now();
  try {
    const arrayBuffer = await file.arrayBuffer();
    activePdfPbnBoards = await extractPbnBoardsFromPdfBuffer(arrayBuffer);

    // Immediately render embedded PBN boards if present so UI responds in <5ms,
    // then upgrade to visual PDF rendering once pdf.js finishes loading.
    if (activePdfPbnBoards.length > 0) {
      activePdfTotalPages = Math.max(1, Math.ceil(activePdfPbnBoards.length / 16));
      await renderActivePdfPage();
    }

    try {
      const pdfjsLib = await loadPdfJs();
      const loadingTask = pdfjsLib.getDocument({ data: new Uint8Array(arrayBuffer) });
      activePdfDoc = await loadingTask.promise;
      activePdfTotalPages = activePdfDoc.numPages;
      activePdfPageNum = 1;
      await renderActivePdfPage();
    } catch (pdfErr) {
      if (activePdfPbnBoards.length === 0) {
        reportRuntimeErrorToSentry(pdfErr, {
          source: 'pdf_render_error',
          file_name: file && file.name ? file.name : ''
        });
        if (scanStatusBadge) {
          scanStatusBadge.className = 'scan-status-badge scan-status-warn';
          scanStatusBadge.textContent = '⚠ Could not render PDF offline and no embedded PBN deals were found.';
        }
      }
    }
    emitSentryMetricDistribution(
      'pdf_parse_ms',
      Number((performance.now() - pdfStartMs).toFixed(1)),
      'millisecond',
      {
        has_pbn: activePdfPbnBoards.length > 0,
        page_count: activePdfTotalPages
      }
    );
  } catch (err) {
    reportRuntimeErrorToSentry(err, {
      source: 'pdf_read_error',
      file_name: file && file.name ? file.name : ''
    });
    if (scanStatusBadge) {
      scanStatusBadge.className = 'scan-status-badge scan-status-warn';
      scanStatusBadge.textContent = '⚠ Failed to read PDF file.';
    }
  }
}

if (pdfPrevPageBtn) {
  pdfPrevPageBtn.addEventListener('click', () => {
    if (activePdfPageNum > 1) {
      activePdfPageNum--;
      renderActivePdfPage();
    }
  });
}

if (pdfNextPageBtn) {
  pdfNextPageBtn.addEventListener('click', () => {
    if (activePdfPageNum < activePdfTotalPages) {
      activePdfPageNum++;
      renderActivePdfPage();
    }
  });
}

function openPhotoScanModalWithFile(file) {
  activePdfDoc = null;
  activePdfPbnBoards = [];
  activePdfPageNum = 1;
  activePdfTotalPages = 1;
  pdfPageBoardCounts = {};
  syncModalContractFromMainUI();
  if (scanBiddingInput) scanBiddingInput.value = '';
  if (scanInitialCardsInput) scanInitialCardsInput.value = '';
  updatePdfPageBarUI();

  const reader = new FileReader();
  reader.onload = (ev) => {
    const img = new Image();
    img.onload = () => {
      const off = document.createElement('canvas');
      off.width = img.naturalWidth || img.width;
      off.height = img.naturalHeight || img.height;
      const ctx = off.getContext('2d');
      ctx.drawImage(img, 0, 0);
      scanSourceCanvas = off;
      if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
      openPhotoScanModalExisting(true);
    };
    img.src = ev.target.result;
  };
  reader.readAsDataURL(file);
}

function openPhotoScanModalExisting(resetDetection = false) {
  if (!scanSourceCanvas || !photoScanModal) return;
  if (resetDetection) {
    syncModalContractFromMainUI();
    if (scanBiddingInput) scanBiddingInput.value = '';
    if (scanInitialCardsInput) scanInitialCardsInput.value = '';
  }
  photoScanModal.classList.remove('hidden');
  updateDeclarerBarUI(currentDeclarerSeat);
  updatePdfPageBarUI();

  requestAnimationFrame(() => {
    if (resetDetection) {
      runFullPhotoBoardDetection();
    } else {
      fitCropPhotoView();
      renderCropCanvas();
      updateModalBiddingUI();
      updateModalContractAndPlayUI();
    }
  });
}

function closePhotoScanModal() {
  pendingModalFileInfo = null;
  if (photoScanModal) photoScanModal.classList.add('hidden');
}

function runFullPhotoBoardDetection(boardOffset = 0) {
  if (!scanSourceCanvas) return;
  const ctx = scanSourceCanvas.getContext('2d');
  const fullData = ctx.getImageData(0, 0, scanSourceCanvas.width, scanSourceCanvas.height);
  const rawBoxes = detectAllBoardsInImageData(fullData);
  detectedBoardBoxes = rawBoxes.map((b, idx) => {
    const pbnEntry = activePdfPbnBoards[boardOffset + idx];
    return {
      ...b,
      boardLabel: pbnEntry ? String(pbnEntry.board) : String(boardOffset + idx + 1),
      pbnHands: pbnEntry ? pbnEntry.hands : null,
      pbnDealer: pbnEntry ? (pbnEntry.dealer || null) : null,
      pbnDeclarer: pbnEntry ? (pbnEntry.declarer || null) : null,
      pbnContract: pbnEntry ? (pbnEntry.contract || null) : null
    };
  });

  if (detectedBoardBoxes.length > 0) {
    activeDetectedBoardIdx = 0;
    lastSelectedBoardLabel = detectedBoardBoxes[0].boardLabel || '1';
    cropBox = { ...detectedBoardBoxes[0] };
    applyBoardMetadataToModal(detectedBoardBoxes[0]);
  } else {
    activeDetectedBoardIdx = -1;
    lastSelectedBoardLabel = '1';
    const padX = Math.round(scanSourceCanvas.width * 0.06);
    const padY = Math.round(scanSourceCanvas.height * 0.06);
    cropBox = {
      x: padX,
      y: padY,
      w: Math.max(40, scanSourceCanvas.width - 2 * padX),
      h: Math.max(40, scanSourceCanvas.height - 2 * padY)
    };
  }

  renderDetectedBoardsPills();
  fitCropPhotoView();
  if (detectedBoardBoxes.length > 1) {
    zoomCropViewToBox();
  }
  scanCurrentCropRegion();
}

function renderDetectedBoardsPills() {
  if (!detectedBoardsBar || !detectedBoardsList) return;
  detectedBoardsList.innerHTML = '';
  if (detectedBoardBoxes.length <= 1 && activePdfTotalPages <= 1) {
    detectedBoardsBar.classList.add('hidden');
    return;
  }
  detectedBoardsBar.classList.remove('hidden');
  detectedBoardBoxes.forEach((box, idx) => {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = `board-pill-btn${idx === activeDetectedBoardIdx ? ' active' : ''}`;
    const label = box.boardLabel || String(idx + 1);
    btn.textContent = `#${label}`;
    btn.addEventListener('click', () => {
      if (activeDetectedBoardIdx !== idx) {
        if (scanBiddingInput) scanBiddingInput.value = '';
        if (scanInitialCardsInput) scanInitialCardsInput.value = '';
      }
      activeDetectedBoardIdx = idx;
      const selectedBox = detectedBoardBoxes[idx];
      lastSelectedBoardLabel = (selectedBox && selectedBox.boardLabel) ? selectedBox.boardLabel : String(idx + 1);
      cropBox = { ...detectedBoardBoxes[idx] };
      applyBoardMetadataToModal(detectedBoardBoxes[idx]);
      renderDetectedBoardsPills();
      zoomCropViewToBox();
      scanCurrentCropRegion();
    });
    detectedBoardsList.appendChild(btn);
  });
}

function fitCropPhotoView() {
  if (!scanSourceCanvas || !cropCanvasViewport || !cropCanvas) return;
  const rect = cropCanvasViewport.getBoundingClientRect();
  const cw = Math.max(260, Math.floor(rect.width));
  const ch = Math.max(220, Math.floor(rect.height));
  cropCanvas.width = cw;
  cropCanvas.height = ch;

  const scaleX = (cw - 24) / scanSourceCanvas.width;
  const scaleY = (ch - 24) / scanSourceCanvas.height;
  cropViewZoom = Math.min(scaleX, scaleY);
  cropViewOffsetX = (cw - scanSourceCanvas.width * cropViewZoom) / 2;
  cropViewOffsetY = (ch - scanSourceCanvas.height * cropViewZoom) / 2;
  renderCropCanvas();
}

function zoomCropViewToBox() {
  if (!scanSourceCanvas || !cropCanvas) return;
  const cw = cropCanvas.width;
  const ch = cropCanvas.height;
  const scaleX = (cw * 0.82) / Math.max(40, cropBox.w);
  const scaleY = (ch * 0.82) / Math.max(40, cropBox.h);
  cropViewZoom = Math.min(scaleX, scaleY);
  const boxCenterX = cropBox.x + cropBox.w / 2;
  const boxCenterY = cropBox.y + cropBox.h / 2;
  cropViewOffsetX = cw / 2 - boxCenterX * cropViewZoom;
  cropViewOffsetY = ch / 2 - boxCenterY * cropViewZoom;
  renderCropCanvas();
}

function clientToImageCoords(clientX, clientY) {
  const rect = cropCanvas.getBoundingClientRect();
  const cx = (clientX - rect.left) * (cropCanvas.width / Math.max(1, rect.width));
  const cy = (clientY - rect.top) * (cropCanvas.height / Math.max(1, rect.height));
  return {
    x: (cx - cropViewOffsetX) / cropViewZoom,
    y: (cy - cropViewOffsetY) / cropViewZoom,
    cx,
    cy
  };
}

function renderCropCanvas() {
  if (!cropCanvas || !scanSourceCanvas) return;
  const ctx = cropCanvas.getContext('2d');
  const cw = cropCanvas.width, ch = cropCanvas.height;
  ctx.clearRect(0, 0, cw, ch);

  ctx.save();
  ctx.translate(cropViewOffsetX, cropViewOffsetY);
  ctx.scale(cropViewZoom, cropViewZoom);
  ctx.drawImage(scanSourceCanvas, 0, 0);
  ctx.restore();

  const bx = cropViewOffsetX + cropBox.x * cropViewZoom;
  const by = cropViewOffsetY + cropBox.y * cropViewZoom;
  const bw = cropBox.w * cropViewZoom;
  const bh = cropBox.h * cropViewZoom;

  // Dim outside cropBox
  ctx.fillStyle = 'rgba(8, 13, 26, 0.62)';
  ctx.fillRect(0, 0, cw, Math.max(0, by));
  ctx.fillRect(0, Math.min(ch, by + bh), cw, Math.max(0, ch - (by + bh)));
  ctx.fillRect(0, Math.max(0, by), Math.max(0, bx), Math.max(0, bh));
  ctx.fillRect(Math.min(cw, bx + bw), Math.max(0, by), Math.max(0, cw - (bx + bw)), Math.max(0, bh));

  // Draw other detected board outlines
  detectedBoardBoxes.forEach((db, idx) => {
    if (idx === activeDetectedBoardIdx) return;
    const dx = cropViewOffsetX + db.x * cropViewZoom;
    const dy = cropViewOffsetY + db.y * cropViewZoom;
    const dw = db.w * cropViewZoom;
    const dh = db.h * cropViewZoom;
    const label = db.boardLabel || String(idx + 1);
    ctx.save();
    ctx.strokeStyle = 'rgba(56, 189, 248, 0.65)';
    ctx.lineWidth = 1.5;
    ctx.setLineDash([5, 4]);
    ctx.strokeRect(dx, dy, dw, dh);
    ctx.fillStyle = 'rgba(2, 132, 199, 0.85)';
    ctx.fillRect(dx + 4, dy + 4, 28, 16);
    ctx.fillStyle = '#ffffff';
    ctx.font = 'bold 10px sans-serif';
    ctx.fillText(`#${label}`, dx + 7, dy + 15);
    ctx.restore();
  });

  // Draw active crop rectangle & subtle 3x3 compass grid
  ctx.save();
  ctx.strokeStyle = 'rgba(148, 163, 184, 0.28)';
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 3]);
  ctx.beginPath();
  ctx.moveTo(bx + bw / 3, by); ctx.lineTo(bx + bw / 3, by + bh);
  ctx.moveTo(bx + (2 * bw) / 3, by); ctx.lineTo(bx + (2 * bw) / 3, by + bh);
  ctx.moveTo(bx, by + bh / 3); ctx.lineTo(bx + bw, by + bh / 3);
  ctx.moveTo(bx, by + (2 * bh) / 3); ctx.lineTo(bx + bw, by + (2 * bh) / 3);
  ctx.stroke();
  ctx.restore();

  // Draw detected N/W/E/S suit anchor markers inside cropBox
  if (lastDetectedAnchors) {
    ctx.save();
    ['N', 'W', 'E', 'S'].forEach(dir => {
      const a = lastDetectedAnchors[dir];
      if (!a) return;
      const ax = bx + a.cx * cropViewZoom;
      const ay = by + a.cy * cropViewZoom;
      ctx.fillStyle = 'rgba(16, 185, 129, 0.9)';
      ctx.beginPath();
      ctx.arc(ax, ay, 5, 0, Math.PI * 2);
      ctx.fill();
    });
    ctx.restore();
  }

  // Active crop border and 8 resize handles
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2;
  ctx.strokeRect(bx, by, bw, bh);

  const handles = [
    [bx, by], [bx + bw / 2, by], [bx + bw, by],
    [bx, by + bh / 2], [bx + bw, by + bh / 2],
    [bx, by + bh], [bx + bw / 2, by + bh], [bx + bw, by + bh]
  ];
  ctx.fillStyle = '#38bdf8';
  ctx.strokeStyle = '#0b1120';
  ctx.lineWidth = 1.5;
  handles.forEach(([hx, hy]) => {
    ctx.beginPath();
    ctx.arc(hx, hy, 5.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
  });
}

function scanCurrentCropRegion() {
  if (!scanSourceCanvas) return;
  const sx = Math.max(0, Math.round(cropBox.x));
  const sy = Math.max(0, Math.round(cropBox.y));
  const sw = Math.min(scanSourceCanvas.width - sx, Math.max(32, Math.round(cropBox.w)));
  const sh = Math.min(scanSourceCanvas.height - sy, Math.max(32, Math.round(cropBox.h)));

  // Normalize crop width to ~420px for consistent OCR resolution
  const targetW = 420;
  const scale = (sw < 340 || sw > 560) ? (targetW / sw) : 1.0;
  const tw = Math.max(64, Math.round(sw * scale));
  const th = Math.max(64, Math.round(sh * scale));

  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = tw;
  tempCanvas.height = th;
  const tCtx = tempCanvas.getContext('2d');
  tCtx.drawImage(scanSourceCanvas, sx, sy, sw, sh, 0, 0, tw, th);
  const imgData = tCtx.getImageData(0, 0, tw, th);

  const ocrStartMs = performance.now();
  const res = recognizeBoardFromImageData(imgData);
  emitSentryMetricDistribution(
    'ocr_scan_ms',
    Number((performance.now() - ocrStartMs).toFixed(1)),
    'millisecond',
    { detected: Boolean(res) }
  );
  const activeBox = (activeDetectedBoardIdx >= 0 && detectedBoardBoxes[activeDetectedBoardIdx])
    ? detectedBoardBoxes[activeDetectedBoardIdx]
    : null;
  const fallbackPbnHands = activeBox && activeBox.pbnHands ? activeBox.pbnHands : null;

  if (!res && !fallbackPbnHands) {
    lastDetectedAnchors = null;
    if (scanStatusBadge) {
      scanStatusBadge.className = 'scan-status-badge scan-status-warn';
      scanStatusBadge.textContent = '⚠ Adjust crop box to frame a single 4-hand board diagram.';
    }
    renderCropCanvas();
    return;
  }

  if (res && res.anchors) {
    lastDetectedAnchors = {};
    ['N', 'W', 'E', 'S'].forEach(dir => {
      lastDetectedAnchors[dir] = {
        cx: res.anchors[dir].cx / scale,
        cy: res.anchors[dir].cy / scale
      };
    });
  } else {
    lastDetectedAnchors = null;
  }

  const useHands = (fallbackPbnHands && (!res || !validateFourHandsDeck(res.hands).isExact52))
    ? fallbackPbnHands
    : (res ? res.hands : fallbackPbnHands);

  if (scanInputN) scanInputN.value = useHands.N;
  if (scanInputE) scanInputE.value = useHands.E;
  if (scanInputS) scanInputS.value = useHands.S;
  if (scanInputW) scanInputW.value = useHands.W;

  updateScanValidationBadge();
  updateModalContractAndPlayUI();
  renderCropCanvas();
}

function updateScanValidationBadge() {
  if (!scanStatusBadge) return;
  const hands = {
    N: scanInputN ? scanInputN.value : '',
    E: scanInputE ? scanInputE.value : '',
    S: scanInputS ? scanInputS.value : '',
    W: scanInputW ? scanInputW.value : ''
  };
  const v = validateFourHandsDeck(hands);
  if (v.isExact52) {
    scanStatusBadge.className = 'scan-status-badge scan-status-ok';
    scanStatusBadge.textContent = '✓ 52/52 cards verified (13 N · 13 E · 13 S · 13 W)';
  } else {
    scanStatusBadge.className = 'scan-status-badge scan-status-warn';
    const dupText = v.duplicates.length > 0 ? ` · Dup: ${v.duplicates.slice(0, 2).join(', ')}` : '';
    scanStatusBadge.textContent =
      `⚠ Card counts: N=${v.counts.N}, E=${v.counts.E}, S=${v.counts.S}, W=${v.counts.W} (${v.uniqueCount}/52 unique)${dupText}`;
  }
}

[scanInputN, scanInputE, scanInputS, scanInputW].forEach(inp => {
  if (inp) {
    inp.addEventListener('input', () => {
      updateScanValidationBadge();
      updateModalContractAndPlayUI();
    });
  }
});

function hitTestCropHandle(cx, cy) {
  const bx = cropViewOffsetX + cropBox.x * cropViewZoom;
  const by = cropViewOffsetY + cropBox.y * cropViewZoom;
  const bw = cropBox.w * cropViewZoom;
  const bh = cropBox.h * cropViewZoom;
  const r = 20;

  if (Math.hypot(cx - bx, cy - by) <= r) return 'nw';
  if (Math.hypot(cx - (bx + bw), cy - by) <= r) return 'ne';
  if (Math.hypot(cx - bx, cy - (by + bh)) <= r) return 'sw';
  if (Math.hypot(cx - (bx + bw), cy - (by + bh)) <= r) return 'se';
  if (Math.hypot(cx - (bx + bw / 2), cy - by) <= r) return 'n';
  if (Math.hypot(cx - (bx + bw / 2), cy - (by + bh)) <= r) return 's';
  if (Math.hypot(cx - bx, cy - (by + bh / 2)) <= r) return 'w';
  if (Math.hypot(cx - (bx + bw), cy - (by + bh / 2)) <= r) return 'e';
  if (cx >= bx && cx <= bx + bw && cy >= by && cy <= by + bh) return 'move';
  return 'pan';
}

function startCropPointer(clientX, clientY) {
  if (!scanSourceCanvas) return;
  const pt = clientToImageCoords(clientX, clientY);
  cropDragMode = hitTestCropHandle(pt.cx, pt.cy);
  cropDragStartImg = { x: pt.x, y: pt.y };
  cropDragStartClient = { x: pt.cx, y: pt.cy, rawX: clientX, rawY: clientY };
  cropDragStartBox = { ...cropBox, offsetX: cropViewOffsetX, offsetY: cropViewOffsetY };
}

function moveCropPointer(clientX, clientY) {
  if (!cropDragMode || !scanSourceCanvas || !cropDragStartBox) return;
  const pt = clientToImageCoords(clientX, clientY);
  const dx = pt.x - cropDragStartImg.x;
  const dy = pt.y - cropDragStartImg.y;
  const minSize = 48;
  const maxW = scanSourceCanvas.width;
  const maxH = scanSourceCanvas.height;

  if (cropDragMode === 'pan') {
    cropViewOffsetX = cropDragStartBox.offsetX + (pt.cx - cropDragStartClient.x);
    cropViewOffsetY = cropDragStartBox.offsetY + (pt.cy - cropDragStartClient.y);
    renderCropCanvas();
    return;
  }

  activeDetectedBoardIdx = -1;
  let { x, y, w, h } = cropDragStartBox;
  if (cropDragMode === 'move') {
    x = Math.max(0, Math.min(maxW - w, x + dx));
    y = Math.max(0, Math.min(maxH - h, y + dy));
  } else {
    if (cropDragMode.includes('w')) {
      const nx = Math.max(0, Math.min(x + w - minSize, x + dx));
      w = (x + w) - nx;
      x = nx;
    }
    if (cropDragMode.includes('e')) {
      w = Math.max(minSize, Math.min(maxW - x, w + dx));
    }
    if (cropDragMode.includes('n')) {
      const ny = Math.max(0, Math.min(y + h - minSize, y + dy));
      h = (y + h) - ny;
      y = ny;
    }
    if (cropDragMode.includes('s')) {
      h = Math.max(minSize, Math.min(maxH - y, h + dy));
    }
  }
  cropBox = { x, y, w, h };
  renderCropCanvas();
}

function endCropPointer(clientX, clientY) {
  if (!cropDragMode) return;
  const mode = cropDragMode;
  cropDragMode = null;

  // Check if user tapped on another detected board box
  if (mode === 'pan' && clientX !== undefined && cropDragStartClient) {
    const moved = Math.hypot(clientX - cropDragStartClient.rawX, clientY - cropDragStartClient.rawY);
    if (moved < 8) {
      const pt = clientToImageCoords(clientX, clientY);
      const hitIdx = detectedBoardBoxes.findIndex(b =>
        pt.x >= b.x && pt.x <= b.x + b.w && pt.y >= b.y && pt.y <= b.y + b.h
      );
      if (hitIdx !== -1) {
        if (activeDetectedBoardIdx !== hitIdx) {
          if (scanBiddingInput) scanBiddingInput.value = '';
          if (scanInitialCardsInput) scanInitialCardsInput.value = '';
        }
        activeDetectedBoardIdx = hitIdx;
        const hitBox = detectedBoardBoxes[hitIdx];
        lastSelectedBoardLabel = (hitBox && hitBox.boardLabel) ? hitBox.boardLabel : String(hitIdx + 1);
        cropBox = { ...detectedBoardBoxes[hitIdx] };
        applyBoardMetadataToModal(detectedBoardBoxes[hitIdx]);
        renderDetectedBoardsPills();
        scanCurrentCropRegion();
        return;
      }
    }
  }

  if (mode !== 'pan') {
    renderDetectedBoardsPills();
    scanCurrentCropRegion();
  }
}

if (cropCanvas) {
  cropCanvas.addEventListener('mousedown', (e) => {
    e.preventDefault();
    startCropPointer(e.clientX, e.clientY);
  });
  window.addEventListener('mousemove', (e) => {
    if (!cropDragMode) return;
    moveCropPointer(e.clientX, e.clientY);
  });
  window.addEventListener('mouseup', (e) => {
    if (!cropDragMode) return;
    endCropPointer(e.clientX, e.clientY);
  });

  cropCanvas.addEventListener('wheel', (e) => {
    if (!scanSourceCanvas) return;
    e.preventDefault();
    const pt = clientToImageCoords(e.clientX, e.clientY);
    const factor = e.deltaY < 0 ? 1.15 : 0.87;
    const newZoom = Math.max(0.15, Math.min(6.0, cropViewZoom * factor));
    cropViewOffsetX = pt.cx - pt.x * newZoom;
    cropViewOffsetY = pt.cy - pt.y * newZoom;
    cropViewZoom = newZoom;
    renderCropCanvas();
  }, { passive: false });

  cropCanvas.addEventListener('touchstart', (e) => {
    if (e.touches.length === 1) {
      e.preventDefault();
      cropPinchDist = 0;
      startCropPointer(e.touches[0].clientX, e.touches[0].clientY);
    } else if (e.touches.length === 2) {
      e.preventDefault();
      cropDragMode = null;
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      cropPinchDist = Math.hypot(dx, dy);
      cropPinchStartZoom = cropViewZoom;
    }
  }, { passive: false });

  cropCanvas.addEventListener('touchmove', (e) => {
    if (e.touches.length === 1 && cropDragMode) {
      e.preventDefault();
      moveCropPointer(e.touches[0].clientX, e.touches[0].clientY);
    } else if (e.touches.length === 2 && cropPinchDist > 0) {
      e.preventDefault();
      const midClientX = (e.touches[0].clientX + e.touches[1].clientX) / 2;
      const midClientY = (e.touches[0].clientY + e.touches[1].clientY) / 2;
      const pt = clientToImageCoords(midClientX, midClientY);
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      const dist = Math.hypot(dx, dy);
      if (dist > 0) {
        const newZoom = Math.max(0.15, Math.min(6.0, cropPinchStartZoom * (dist / cropPinchDist)));
        cropViewOffsetX = pt.cx - pt.x * newZoom;
        cropViewOffsetY = pt.cy - pt.y * newZoom;
        cropViewZoom = newZoom;
        renderCropCanvas();
      }
    }
  }, { passive: false });

  cropCanvas.addEventListener('touchend', (e) => {
    if (cropDragMode) {
      const t = (e.changedTouches && e.changedTouches[0]) ? e.changedTouches[0] : null;
      endCropPointer(t ? t.clientX : undefined, t ? t.clientY : undefined);
    }
    cropPinchDist = 0;
  });
}

if (cropRotateBtn) {
  cropRotateBtn.addEventListener('click', () => {
    if (!scanSourceCanvas) return;
    const rot = document.createElement('canvas');
    rot.width = scanSourceCanvas.height;
    rot.height = scanSourceCanvas.width;
    const rCtx = rot.getContext('2d');
    rCtx.translate(rot.width / 2, rot.height / 2);
    rCtx.rotate(Math.PI / 2);
    rCtx.drawImage(scanSourceCanvas, -scanSourceCanvas.width / 2, -scanSourceCanvas.height / 2);
    scanSourceCanvas = rot;
    runFullPhotoBoardDetection();
  });
}

if (cropZoomToBoxBtn) {
  cropZoomToBoxBtn.addEventListener('click', zoomCropViewToBox);
}

if (cropResetViewBtn) {
  cropResetViewBtn.addEventListener('click', fitCropPhotoView);
}

if (cropNewPhotoBtn && fileInputTxt) {
  cropNewPhotoBtn.addEventListener('click', () => {
    fileInputTxt.value = '';
    fileInputTxt.click();
  });
}

if (rescanCropBtn) {
  rescanCropBtn.addEventListener('click', scanCurrentCropRegion);
}

document.querySelectorAll('.modal-declarer-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    updateDeclarerBarUI(btn.dataset.seat);
  });
});

document.querySelectorAll('.modal-contract-level-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const lvl = parseInt(btn.dataset.level, 10);
    if (!isNaN(lvl) && lvl >= 1 && lvl <= 7) {
      modalContractLevel = lvl;
      updateModalContractAndPlayUI();
    }
  });
});

document.querySelectorAll('.modal-contract-strain-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    if (btn.dataset.strain) {
      modalContractStrain = btn.dataset.strain;
      updateModalContractAndPlayUI();
    }
  });
});

document.querySelectorAll('.modal-dealer-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const btnDealer = btn.dataset.dealer || btn.dataset.seat;
    if (btnDealer && ['N', 'E', 'S', 'W'].includes(btnDealer)) {
      modalDealerSeat = btnDealer;
      updateModalBiddingUI();
    }
  });
});

if (scanBiddingInput) {
  scanBiddingInput.addEventListener('input', () => {
    updateModalBiddingUI();
  });
}

if (scanUndoBidBtn && scanBiddingInput) {
  scanUndoBidBtn.addEventListener('click', () => {
    const toks = scanBiddingInput.value.trim().split(/[\s,;/]+/).filter(Boolean);
    if (toks.length > 0) {
      toks.pop();
      scanBiddingInput.value = toks.join(' ');
      updateModalBiddingUI();
    }
  });
}

if (scanClearBidsBtn && scanBiddingInput) {
  scanClearBidsBtn.addEventListener('click', () => {
    scanBiddingInput.value = '';
    updateModalBiddingUI();
  });
}

if (scanInitialCardsInput) {
  scanInitialCardsInput.addEventListener('input', () => {
    updateModalContractAndPlayUI();
  });
}

if (scanUndoCardBtn && scanInitialCardsInput) {
  scanUndoCardBtn.addEventListener('click', () => {
    const toks = scanInitialCardsInput.value.trim().split(/[\s,;/]+/).filter(Boolean);
    if (toks.length > 0) {
      toks.pop();
      scanInitialCardsInput.value = toks.join(' ');
      updateModalContractAndPlayUI();
    }
  });
}

if (scanClearCardsBtn && scanInitialCardsInput) {
  scanClearCardsBtn.addEventListener('click', () => {
    scanInitialCardsInput.value = '';
    updateModalContractAndPlayUI();
  });
}

if (closePhotoModalBtn) closePhotoModalBtn.addEventListener('click', closePhotoScanModal);
if (cancelPhotoModalBtn) cancelPhotoModalBtn.addEventListener('click', closePhotoScanModal);

if (applyPhotoModalBtn) {
  applyPhotoModalBtn.addEventListener('click', () => {
    if (pendingModalFileInfo) {
      activeLoadedFileInfo = pendingModalFileInfo;
      pendingModalFileInfo = null;
    }

    loadedBoardHands = {
      N: (scanInputN ? scanInputN.value : '').trim(),
      E: (scanInputE ? scanInputE.value : '').trim(),
      S: (scanInputS ? scanInputS.value : '').trim(),
      W: (scanInputW ? scanInputW.value : '').trim()
    };
    closePhotoScanModal();

    let activeBox = (activeDetectedBoardIdx >= 0 && detectedBoardBoxes[activeDetectedBoardIdx])
      ? detectedBoardBoxes[activeDetectedBoardIdx]
      : null;

    if (!activeBox && cropBox && detectedBoardBoxes && detectedBoardBoxes.length > 0) {
      let bestOverlap = 0;
      let bestBox = null;
      for (const box of detectedBoardBoxes) {
        const overlapX = Math.max(0, Math.min(cropBox.x + cropBox.w, box.x + box.w) - Math.max(cropBox.x, box.x));
        const overlapY = Math.max(0, Math.min(cropBox.y + cropBox.h, box.y + box.h) - Math.max(cropBox.y, box.y));
        const overlapArea = overlapX * overlapY;
        if (overlapArea > bestOverlap) {
          bestOverlap = overlapArea;
          bestBox = box;
        }
      }
      if (bestBox && bestOverlap > 0) {
        activeBox = bestBox;
      }
    }

    const boardLabel = activeBox && activeBox.boardLabel
      ? activeBox.boardLabel
      : (lastSelectedBoardLabel || String(activeDetectedBoardIdx >= 0 ? activeDetectedBoardIdx + 1 : 1));

    let sourceMeta = null;
    if (activeLoadedFileInfo) {
      sourceMeta = {
        fileName: activeLoadedFileInfo.fileName,
        boardLabel: boardLabel,
        fileFormat: activeLoadedFileInfo.fileFormat || 'pdf',
        loadType: activeLoadedFileInfo.loadType || 'pdf'
      };
    } else if (loadedHandSource) {
      sourceMeta = {
        ...loadedHandSource,
        boardLabel: boardLabel
      };
    } else {
      sourceMeta = {
        fileName: 'imported_deal',
        boardLabel: boardLabel,
        fileFormat: 'pdf',
        loadType: 'file_import'
      };
    }

    applyDeclarerRotation(currentDeclarerSeat, true);
    registerSessionCustomHand(sourceMeta);
    emitSentryMetricCount('hand_loaded', 1, {
      load_type: (sourceMeta && sourceMeta.loadType) || 'file_import'
    });
  });
}
