using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;

namespace ScriptApp {
	class Program {
		bool running = true;

		static int Main(string[] args) {
			return 0;//new Program().Run();
		}

		public int Run() {
			Console.WriteLine("Hello!");

			while (running) {
				Console.Write("> ");

				string line = Console.ReadLine();

				if (line.Equals ("r")) {
					reload(); // FireOnReload() -> OnReload() -> running = false
					break;
				} else if (line.Equals("e")) {
					throw new System.InvalidOperationException("INVALID OPERATION");
				} else if (line.Equals("q")) {
					Console.WriteLine("Bye bye!");
					return 0;
				} else {
					Console.WriteLine(line);
				}
			}
			return 1; // return non zero exit code for reloading
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern static void reload();

		public void OnReload() {
			running = false;
		}
	}
}
