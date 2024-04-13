// Alter's steam ID: 76561198073630548
using BepInEx;
using BepInEx.Logging;
using HarmonyLib;


namespace LethalRecordingMod
{
	[BepInPlugin("TheNerds.LethalRecordingMod", "LethalRecordingMod", "0.0.1")]
	public class Plugin : BaseUnityPlugin
	{
		private readonly Harmony harmony = new Harmony("TheNerds.LethalRecordingMod");
		public static Plugin Instance;
		public ManualLogSource mls;
		private void Awake()
		{
			if (Instance == null) {
				Instance = this;
			} 
			// Plugin startup logic
			Logger.LogInfo($"Plugin LethalRecordingMod hath become loaded!");
			mls.LogInfo("We're up and running!");
		}
	}


	enum TrollSettingType
	{
		Undefined,
		Toggle,
		Value,
		Person,
		IGT, // In-game time
		RLT // Real-life time
	}


	public class TrollSettingValue
	{
		public TrollSettingType type;
		public bool toggle;
		public double val;
		public long person;
		public double IGT; // 0-1, 0 = start of the day, 1 = midnight
		public byte RLT_month; // Don't know why you'd want this
		public byte RLT_mday; // Day of the month
		public byte RLT_wday; // Day of the week
		public byte RLT_hour; // Hour
		public byte RLT_minute; // Minute
		public byte RLT_second; // Second, optional
		public short RLT_millisecond; // Millisecond, optional
	}


	public class TrollSetting
	{
		public string name = "Name your settings, guys!";
		public TrollSettingValue val;
		public TrollSetting(TrollSettingType type, string newName)
		{
			val.type = type;
			name = newName;
		}
	}


	public class Troll // Base class
	{
		public bool repeatable = true;
		public int timesOccured = 0;
		public bool clientSide = false;
		public long targetSteamID;
		public bool active;
		public List<TrollSetting> settings;
		public int OnTrigger() {} // Should be overwritten by most trolls, returns 0 on success, should be used instead of Start or Awake to be toggleable
		public int OnStop() {}
		public int WhileActive(double deltaTime) {} // Should be used instead of Update, to make troll able to be disabled
		public ref TrollSetting GetSetting(string name)
		{
			for (int i = 0; i < settings.Count; i++)
			{
				if (settings[i].name == name)
				{
					return ref settings[i];
				}
			}
		}
	}
	public class IntervalTroll : Troll // Still base class
	{
		public static const int defaultInterval = -1; // Seconds
		public int timer = 0;
		public static const double defaultChance = 100; // Percentage
		public IntervalTroll()
		{
			timer = 0;
			settings.Add(new TrollSetting(TrollSettingType.Value, "Interval"));
			settings.Add(new TrollSetting(TrollSettingType.Value, "Chance"));
		}
		public override int WhileActive(double deltaTime)
		{
			timer += deltaTime;
			if (timer >= GetSetting("Interval").val.val)
			{
				timer = 0;
				if ((new Random()).Next(0, 100) < GetSetting("Chance").val.val)
				{
					OnTrigger();
				}
			}
		}
	}
}
