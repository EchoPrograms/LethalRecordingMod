// Alter's steam ID: 76561198073630548
using BepInEx;
using BepInEx.Logging;
using HarmonyLib;
using System.Collections.Generic;

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


	public abstract class Troll // Base class
	{
		public bool repeatable = true;
		public int timesOccured = 0;
		public bool clientSide = false;
		public long targetSteamID;
		public bool active;
		public abstract int OnTrigger(); // Should be overwritten by most trolls, returns 0 on success, should be used instead of Start or Awake to be toggleable
		public abstract int OnStop();
		public abstract int WhileActive(); // Should be used instead of Update, to make troll able to be disabled

	}
	public class IntervalTroll : Troll // Still base class
	{
		public const int defaultInterval = -1; // Seconds
		public const double defaultChance = 100; // Percentage
		public IntervalTroll()
		{

		}
        public override int OnStop()
        {
			return 0;
        }
        public override int OnTrigger()
        {
			return 0;
        }
        public override int WhileActive()
		{
			return 0;
		}
	}
}
