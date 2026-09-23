module.exports = [
  {
    type: "heading",
    defaultValue: "NRL Fan"
  },
  {
    type: "text",
    defaultValue: "Pick your sides. The watch uses these for Results and the home header."
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Teams"
      },
      {
        type: "select",
        messageKey: "FAV_NRL",
        label: "NRL team",
        defaultValue: "Broncos",
        options: [
          { label: "Brisbane Broncos", value: "Broncos" },
          { label: "Canberra Raiders", value: "Raiders" },
          { label: "Canterbury-Bankstown Bulldogs", value: "Bulldogs" },
          { label: "Cronulla-Sutherland Sharks", value: "Sharks" },
          { label: "Dolphins", value: "Dolphins" },
          { label: "Gold Coast Titans", value: "Titans" },
          { label: "Manly-Warringah Sea Eagles", value: "Sea Eagles" },
          { label: "Melbourne Storm", value: "Storm" },
          { label: "Newcastle Knights", value: "Knights" },
          { label: "New Zealand Warriors", value: "Warriors" },
          { label: "North Queensland Cowboys", value: "Cowboys" },
          { label: "Parramatta Eels", value: "Eels" },
          { label: "Penrith Panthers", value: "Panthers" },
          { label: "South Sydney Rabbitohs", value: "Rabbitohs" },
          { label: "St George Illawarra Dragons", value: "Dragons" },
          { label: "Sydney Roosters", value: "Roosters" },
          { label: "Wests Tigers", value: "Wests Tigers" }
        ]
      },
      {
        type: "select",
        messageKey: "FAV_NRLW",
        label: "NRLW team",
        defaultValue: "Broncos",
        options: [
          { label: "Brisbane Broncos", value: "Broncos" },
          { label: "Canberra Raiders", value: "Raiders" },
          { label: "Canterbury-Bankstown Bulldogs", value: "Bulldogs" },
          { label: "Cronulla-Sutherland Sharks", value: "Sharks" },
          { label: "Gold Coast Titans", value: "Titans" },
          { label: "Newcastle Knights", value: "Knights" },
          { label: "New Zealand Warriors", value: "Warriors" },
          { label: "North Queensland Cowboys", value: "Cowboys" },
          { label: "Parramatta Eels", value: "Eels" },
          { label: "St George Illawarra Dragons", value: "Dragons" },
          { label: "Sydney Roosters", value: "Roosters" },
          { label: "Wests Tigers", value: "Wests Tigers" }
        ]
      },
      {
        type: "select",
        messageKey: "FAV_ORIGIN",
        label: "Origin side",
        defaultValue: "NSW",
        options: [
          { label: "New South Wales Blues", value: "NSW" },
          { label: "Queensland Maroons", value: "QLD" }
        ]
      }
    ]
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Launch"
      },
      {
        type: "select",
        messageKey: "DEFAULT_COMP",
        label: "Default competition",
        defaultValue: "0",
        options: [
          { label: "NRL", value: "0" },
          { label: "NRLW", value: "1" },
          { label: "State of Origin", value: "2" },
          { label: "Women's Origin", value: "3" }
        ]
      }
    ]
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Odds"
      },
      {
        type: "text",
        defaultValue: "Off shows win chance %. On shows NRL decimal odds (e.g. 1.51)."
      },
      {
        type: "toggle",
        messageKey: "ODDS_RAW",
        label: "Odds type",
        defaultValue: false
      }
    ]
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Timeline"
      },
      {
        type: "select",
        messageKey: "PIN_REMINDER",
        label: "Pin reminder",
        defaultValue: "60",
        options: [
          { label: "Off", value: "0" },
          { label: "15 minutes before", value: "15" },
          { label: "60 minutes before", value: "60" },
          { label: "15 and 60 minutes before", value: "15+60" }
        ]
      }
    ]
  },
  {
    type: "submit",
    defaultValue: "Save"
  }
];
