// Alter's steam ID: 76561198073630548
using BepInEx;
using BepInEx.Logging;
using HarmonyLib;
using System.Collections.Generic;
using UnityEngine;
using System;
namespace LethalRecordingMod
{
	[BepInPlugin("TheNerds.LethalRecordingMod", "LethalRecordingMod", "0.0.1")]
	public class Plugin : BaseUnityPlugin
	{
		private readonly Harmony harmony = new Harmony("TheNerds.LethalRecordingMod");
		public static Plugin instance;
		public IntervalTroll testTroll;
		public static ManualLogSource logger;
		private void Awake()
		{
			if (instance == null) {
                instance = this;
            }
            testTroll = new IntervalTroll(0.01, 100);
            logger = Logger;
            logger.LogInfo($"Plugin LethalRecordingMod hath become loaded!");
		}


        public Action updateCallback;
        private void Update()
        {
            updateCallback();
        }
    }


	public abstract class Troll // Base class
	{
		public bool repeatable = true;
		public int timesOccured = 0;
		public bool clientSide = false;
		public long targetSteamID;
		private bool active = true;
		public Troll()
		{
            Plugin.instance.updateCallback += OnUpdate;
        }
		public abstract int OnTrigger(); // Should be overwritten by most trolls, returns 0 on success, should be used instead of Start or Awake to be toggleable
		public abstract int OnStop();
		public abstract int OnStart();

		public bool GetStatus()
		{
			return active;
		}
		public virtual int Start()
		{
			active = true;
			return OnStart();
		}
		public virtual int Stop()
		{
			active = false;
			return OnStop();
		}
		public abstract int WhileActive(); // Should be used instead of Update, to make troll able to be disabled
		private void OnUpdate()
		{
			if(active)
			{
				WhileActive();
            } 
		}
	}
	public class IntervalTroll : Troll // Still base class
	{
		public double interval; // Seconds
		public double chance; // Percentage
		private double timer = 0;


        public IntervalTroll(double interval = 1, double chance = 100)
		{
			this.chance = chance;
			this.interval = interval;
        }
        public override int OnStop()
        {
			return 0;
        }
        public override int OnStart()
        {
			return 0;
        }
        public override int OnTrigger()
        {
            Plugin.logger.LogFatal("Pranked");
			return 0;
        }
        public override int WhileActive()
		{
            timer += Time.deltaTime;
			if(timer > interval) {
				timer = 0;
                if (UnityEngine.Random.Range(0f, 100f) < chance)
				{
					OnTrigger();
				}
            }

            return 0;
		}
	}
	public class ControlPanel // For interacting with every troll and spawning in new trolls
	{
		public ControlPanel(long steamID)
		{
			
		}
	}
}

