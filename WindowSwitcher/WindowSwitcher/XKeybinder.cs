/*****************************************************************************/
/* XKeybinder.cs - Keybinding code taken from Tomboy			     */
/* Copyright (C) 2004-2007 Alex Graveley <alex@beatniksoftware.com>	     */
/* 									     */
/* This library is free software; you can redistribute it and/or	     */
/* modify it under the terms of the GNU Lesser General Public		     */
/* License as published by the Free Software Foundation; either		     */
/* version 2.1 of the License, or (at your option) any later version.	     */
/* 									     */
/* This library is distributed in the hope that it will be useful,	     */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU	     */
/* Lesser General Public License for more details.			     */
/* 									     */
/* You should have received a copy of the GNU Lesser General Public	     */
/* License along with this library; if not, write to the Free Software	     */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*****************************************************************************/

using System;
using System.Collections;
using System.Runtime.InteropServices;

namespace WindowSwitcher
{
	public class XKeybinder 
	{
		const string libdo = "libwsbind";
		
		[DllImport(libdo)]
		static extern void wsbind_keybinder_init ();

		[DllImport(libdo)]
		static extern void wsbind_keybinder_bind (string keystring, BindkeyHandler handler);

		[DllImport(libdo)]
		static extern void wsbind_keybinder_unbind (string keystring, BindkeyHandler handler);

		public delegate void BindkeyHandler (string key, IntPtr user_data);

		ArrayList      bindings;
		BindkeyHandler key_handler;

		struct Binding {
			internal string       keystring;
			internal EventHandler handler;
		}

		public XKeybinder ()
			: base ()
		{
			bindings = new ArrayList ();
			key_handler = new BindkeyHandler (KeybindingPressed);
			
			try {
				wsbind_keybinder_init ();
			} catch (DllNotFoundException) {
				Console.Error.WriteLine ("libdo not found - keybindings will not work.");
			}
		}

		void KeybindingPressed (string keystring, IntPtr user_data)
		{
			foreach (Binding bind in bindings) {
				if (bind.keystring == keystring) {
					bind.handler (this, new EventArgs ());
				}
			}
		}

		public void Bind (string keystring, EventHandler handler)
		{
			Binding bind = new Binding ();
			bind.keystring = keystring;
			bind.handler = handler;
			bindings.Add (bind);
			
			wsbind_keybinder_bind (bind.keystring, key_handler);
		}

		public void Unbind (string keystring)
		{
			foreach (Binding bind in bindings) {
				if (bind.keystring == keystring) {
					wsbind_keybinder_unbind (bind.keystring, key_handler);

					bindings.Remove (bind);
					break;
				}
			}
		}

		public virtual void UnbindAll ()
		{
			foreach (Binding bind in bindings) {
				wsbind_keybinder_unbind (bind.keystring, key_handler);
			}

			bindings.Clear ();
		}
	}
}
