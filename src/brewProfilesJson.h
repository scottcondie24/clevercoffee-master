#pragma once

const char* defaultProfilesJson = R"json(
[
  {
    "name": "Spring Lever",
    "shortname": "springLever",
    "temperature": 90,
    "scales": false,
    "flow": true,
    "phases": [
      {
        "name": "infuse",
        "flow": 8,
        "exit_flow_over": 4,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "rise and hold",
        "pressure": 8.6,
        "seconds": 4,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 6,
        "max_secondary": 6,
        "seconds": 30,
        "exit_type": "pressure_under",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "maintain flow",
        "flow": 1.5,
        "seconds": 30,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Adaptive",
    "shortname": "adaptive",
    "temperature": 88,
    "scales": true,
    "flow": true,
    "phases": [
      {
        "name": "pause",
        "seconds": 1,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "fill",
        "flow": 8,
        "exit_pressure_over": 3,
        "weight": 5,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "infuse",
        "pressure": 3,
        "flow": 1.7,
        "weight": 4,
        "max_secondary": 8.5,
        "max_secondary_range": 5,
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "maintain flow",
        "flow": 1.7,
        "weight": 50,
        "max_secondary": 8.6,
        "max_secondary_range": 0.5,
        "seconds": 80,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Londinium R24",
    "shortname": "londiniumR",
    "temperature": 88,
    "scales": true,
    "flow": true,
    "phases": [
      {
        "name": "pause",
        "seconds": 1,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "fill",
        "flow": 12,
        "exit_pressure_over": 2.5,
        "weight": 5,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "infuse",
        "pressure": 3,
        "flow": 1.7,
        "weight": 4,
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "rise and hold",
        "pressure": 8.6,
        "seconds": 2,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 5,
        "weight": 36,
        "seconds": 45,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "end",
        "flow": 0.1,
        "weight": 38,
        "max_secondary": 8,
        "max_secondary_range": 0.5,
        "seconds": 5,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Londinium Vectis",
    "shortname": "londiniumV",
    "temperature": 88,
    "scales": true,
    "flow": true,
    "phases": [
      {
        "name": "pause",
        "seconds": 1,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "fill",
        "flow": 12,
        "exit_pressure_over": 1,
        "weight": 5,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "infuse",
        "pressure": 1.2,
        "flow": 1.7,
        "weight": 4,
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "rise and hold",
        "pressure": 6.5,
        "seconds": 2,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 4,
        "weight": 36,
        "seconds": 45,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "end",
        "flow": 0.1,
        "weight": 38,
        "seconds": 5,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Londinium R24 Pressure Only",
    "shortname": "londiniumRPressure",
    "temperature": 88,
    "scales": true,
    "flow": false,
    "phases": [
      {
        "name": "pause",
        "seconds": 1,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "fill",
        "pressure": 3,
        "weight": 5,
        "exit_pressure_over": 2.5,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "infuse",
        "pressure": 3,
        "weight": 4,
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "rise and hold",
        "pressure": 8.6,
        "seconds": 2,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 5,
        "weight": 36,
        "seconds": 45,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "end",
        "pressure": 0.1,
        "weight": 38,
        "seconds": 5,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      }
    ]
  },
  {
    "name": "Londinium Vectis Pressure Only",
    "shortname": "londiniumVPressure",
    "temperature": 88,
    "scales": true,
    "flow": false,
    "phases": [
      {
        "name": "pause",
        "seconds": 1,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "fill",
        "pressure": 3,
        "weight": 5,
        "exit_pressure_over": 1,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "infuse",
        "pressure": 1.2,
        "weight": 4,
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "rise and hold",
        "pressure": 6.5,
        "seconds": 2,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 4,
        "weight": 36,
        "seconds": 45,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "end",
        "pressure": 0.1,
        "weight": 38,
        "seconds": 5,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      }
    ]
  },
  {
    "name": "Light Roast",
    "shortname": "lightRoast",
    "temperature": 92,
    "scales": true,
    "flow": true,
    "phases": [
      {
        "name": "fill",
        "pressure": 3.5,
        "exit_pressure_over": 3,
        "seconds": 12,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "fill end",
        "pressure": 3,
        "exit_flow_under": 1,
        "seconds": 12,
        "exit_type": "flow_under",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "infuse",
        "pressure": 1.5,
        "weight": 4.0,
        "seconds": 13,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "pressure",
        "pressure": 10,
        "exit_pressure_over": 8,
        "seconds": 6,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "extract",
        "flow": 3.4,
        "seconds": 30,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Six Bar Espresso",
    "shortname": "sixBarEspresso",
    "temperature": 90,
    "scales": true,
    "flow": true,
    "phases": [
      {
        "name": "infuse",
        "flow": 8,
        "exit_pressure_over": 4,
        "seconds": 20,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "rise and hold",
        "pressure": 6,
        "seconds": 16,
        "exit_type": "none",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "decline",
        "pressure": 4,
        "weight": 36,
        "exit_pressure_under": 4,
        "seconds": 30,
        "exit_type": "pressure_under",
        "transition": "smooth",
        "pump": "pressure"
      }
    ]
  },
  {
    "name": "Blooming Espresso",
    "shortname": "bloomingEspresso",
    "temperature": 92,
    "scales": false,
    "flow": true,
    "phases": [
      {
        "name": "infuse",
        "flow": 4,
        "exit_pressure_over": 1,
        "seconds": 23,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "pause",
        "seconds": 30,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "ramp",
        "flow": 2.2,
        "seconds": 5,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      },
      {
        "name": "flow",
        "flow": 2.2,
        "seconds": 20,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Pressurized Bloom",
    "shortname": "pressurizedBloom",
    "temperature": 93,
    "scales": false,
    "flow": true,
    "phases": [
      {
        "name": "fill",
        "flow": 8,
        "exit_pressure_over": 3,
        "seconds": 15,
        "exit_type": "pressure_over",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "pressure",
        "pressure": 3,
        "exit_flow_under": 0.9,
        "seconds": 12,
        "exit_type": "flow_under",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "hold",
        "pressure": 0.1,
        "exit_flow_over": 1,
        "seconds": 6,
        "exit_type": "flow_over",
        "transition": "fast",
        "pump": "pressure"
      },
      {
        "name": "pressure",
        "pressure": 11,
        "exit_pressure_over": 8.8,
        "seconds": 6,
        "exit_type": "pressure_over",
        "transition": "smooth",
        "pump": "pressure"
      },
      {
        "name": "extract",
        "flow": 3.5,
        "seconds": 60,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Calibrate Flow",
    "shortname": "calibrateFlow",
    "temperature": 90,
    "scales": false,
    "flow": true,
    "phases": [
      {
        "name": "2.0mL/s",
        "flow": 2,
        "seconds": 8,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "4.0mL/s",
        "flow": 4,
        "seconds": 8,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "6.0mL/s",
        "flow": 6,
        "seconds": 8,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      },
      {
        "name": "8.0mL/s",
        "flow": 8,
        "seconds": 8,
        "exit_type": "none",
        "transition": "fast",
        "pump": "flow"
      }
    ]
  },
  {
    "name": "Test Ramp Flow",
    "shortname": "testRampFlow",
    "temperature": 90,
    "scales": false,
    "flow": true,
    "phases": [
      {
        "name": "increasing flow",
        "flow": 4,
        "seconds": 4,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      },
      {
        "name": "decreasing flow",
        "flow": 1,
        "seconds": 4,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      },
      {
        "name": "increasing flow",
        "flow": 6,
        "seconds": 4,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      },
      {
        "name": "decreasing flow",
        "flow": 0,
        "seconds": 4,
        "exit_type": "none",
        "transition": "smooth",
        "pump": "flow"
      }
    ]
  }
]
)json";