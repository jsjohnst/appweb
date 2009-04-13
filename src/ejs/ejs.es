
/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Embedthis Ejscript 0.9.8.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify ejs, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../es/core/Array.es"
 */
/************************************************************************/

/**
 *	Array.es - Array class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
	 *	TODO 
	 *		- ECMA should be dynamic
	 *		- removeByElement(e) - remove an array element by reference
	 */

	/**
	 *	Arrays provide a growable, integer indexed, in-memory store for objects. An array can be treated as a 
	 *	stack (FIFO or LIFO) or a list (ordered). Insertions can be done at the beginning or end of the stack or at an 
	 *	indexed location within a list.
	 *	@spec ecma-262
	 */
	dynamic native class Array {

        use default namespace public

		/**
		 *	Create a new array.
		 *	@param values Initialization values. The constructor allows three forms:
		 *	<ul>
		 *		<li>Array()</li>
		 *		<li>Array(size: Number)</li>
		 *		<li>Array(elt: Object, ...args)</li>
		 *	</ul>
		 *	@spec ecma-3
		 */
		native function Array(...values)


		/**
		 *	Append an item to the array.
		 *	@param obj Object to add to the array 
		 *	@return The array itself.
		 *	@spec ejs-11
		 */
		native function append(obj: Object): Array


		/**
		 *	Clear an array. Remove all elements of the array.
		 *	@spec ejs-11
		 */
		native function clear() : Void


		/**
		 *	Clone the array and all its elements.
		 *	@param deep If true, do a deep copy where all object references are also copied, and so on, recursively.
		 *	@spec ejs-11
		 */
		override native function clone(deep: Boolean = true) : Array


		/**
		 *	Compact an array. Remove all null elements.
		 *	@spec ejs-11
		 */
		native function compact() : Array


		/**
		 *	Concatenate the supplied elements with the array to create a new array. If any arguments specify an 
		 *	array, their elements are concatenated. This is a one level deep copy.
		 *	@param args A variable length set of values of any data type.
		 *	@return A new array of the combined elements.
		 *	@spec ecma-3
		 */
		native function concat(...args): Array 


		/**
		 *	See if the array contains an element using strict equality "===". This call searches from the start of the 
         *	array for the specified element.  
		 *	@param element The object to search for.
		 *	@return True if the element is found. Otherwise false.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ejs-11
		 */
		function contains(element: Object): Boolean {
            if (indexOf(element) >= 0) {
                return true
            } else {
                return false
            }
        }


		/**
		 *	Determine if all elements match.
		 *	Iterate over every element in the array and determine if the matching function is true for all elements. 
		 *	This method is lazy and will cease iterating when an unsuccessful match is found. The checker is called 
		 *	with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True if the match function always returns true.
		 *	@spec ecma-3
		 */
		function every(match: Function): Boolean {
			for (let i: Number in this) {
				if (!match(this[i], i, this)) {
					return false
				}
			}
			return true
		}


		 //	TODO - how does this compare with JS1.8 reduce?
		/**
		 *	Find all matching elements. ECMA function doing the Same as findAll.
		 *	Iterate over all elements in the object and find all elements for which the matching function is true.
		 *	The match is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	This method is identical to the @findAll method.
		 *	@param match Matching function
		 *	@return Returns a new array containing all matching elements.
		 *	@spec ecma-3
		 */
		function filter(match: Function): Array {
			return findAll(match)
		}


		 //	TODO - should this return the index rather than the element?
		/**
		 *	Find the first matching element.
		 *	Iterate over elements in the object and select the first element for which the matching function is true.
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True when the match function returns true.
		 *	@spec ejs-11
		 */
		function find(match: Function): Object {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					return this[i]
				}
			}
			return result
		}


		/**
		 *	Find all matching elements.
		 *	Iterate over all elements in the object and find all elements for which the matching function is true.
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return Returns an array containing all matching elements.
		 *	@spec ejs-11
		 */
		function findAll(match: Function): Array {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					result.append(this[i])
				}
			}
			return result
		}


		/**
		 *	Transform all elements.
		 *	Iterate over the elements in the array and transform all elements by applying the transform function. 
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	This method is identical to the @transform method.
		 *	@param modifier Transforming function
		 *	@return Returns a new array of transformed elements.
		 *	@spec ecma-3
		 */
		function forEach(modifier: Function, thisObj: Object = null): Void {
			transform(modifier)
		}


		/**
		 *	Get an iterator for this array to be used by "for (v in array)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in array)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Search for an item using strict equality "===". This call searches from the start of the array for 
		 *	the specified element.  
		 *	@param element The object to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function indexOf(element: Object, startIndex: Number = 0): Number


		/**
		 *	Insert elements. Insert elements at the specified position. The insertion occurs before the element at the 
		 *		specified position. Negative indicies will measure from the end so that -1 will specify the last element.  
		 *	@param pos Where in the array to insert the objects.
		 *	@param ...args Arguments are aggregated and passed to the method in an array.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@return The original array.
		 *	@spec ejs-11
		 */
		native function insert(pos: Number, ...args): Array


		/**
		 *	Convert the array into a string.
		 *	Joins the elements in the array into a single string by converting each element to a string and then 
		 *	concatenating the strings inserting a separator between each.
		 *	@param sep Element separator.
		 *	@return A string.
		 *	@spec ecma-3
		 */
		native function join(sep: String = undefined): String


		/**
		 *	Find an item searching from the end of the arry.
		 *	Search for an item using strict equality "===". This call searches from the end of the array for the given 
         *	element.
		 *	@param element The object to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function lastIndexOf(element: Object, fromIndex: Number = 0): Number


		/**
		 *	Get the length of an array.
		 *	@return The number of items in the array
		 */
		override native function get length(): Number


		/**
		 *	Set the length of an array. The array will be truncated if required. If the new length is greater then 
		 *	the old length, new elements will be created as required and set to undefined. If the new length is less
		 *	than 0 the length is set to 0.
		 *	@param value The new length
		 */
		native function set length(value: Number): Void


		/**
		 *	Call the mapper on each array element in increasing index order and return a new array with the values returned 
		 *	from the mapper. The mapper function is called with the following signature:
		 *		function mapper(element: Object, elementIndex: Number, arr: Array): Object
		 *	@param mapper Transforming function
		 *	@spec ecma-3
		 */
		function map(mapper: Function): Array {
			//	BUG TODO return clone().transform(mapper)
			var result: Array  = clone()
			result.transform(mapper)
			return result
		}


		/**
		 *	Remove and return the last value in the array.
		 *	@return The last element in the array. Returns undefined if the array is empty
		 *	@spec ecma-3
		 */
		native function pop(): Object 


		/**
		 *	Append items to the end of the array.
		 *	@param items Items to add to the array.
		 *	@return The new length of the array.
		 *	@spec ecma-3
		 */
		native function push(...items): Number 


		/**
		 *	Find non-matching elements. Iterate over all elements in the array and select all elements for which 
		 *	the filter function returns false. The matching function is called with the following signature:
		 *
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *
		 *	@param match Matching function
		 *	@return A new array of non-matching elements.
		 *	@spec ejs-11
		 */
		function reject(match: Function): Array {
			var result: Array = new Array
			for (let i: Number in this) {
				if (!match(this[i], i, this)) {
					result.append(this[i])
				}
			}
			return result
		}


		/*
		 *	Remove elements. Remove the elements from $start to $end inclusive. The elements are removed and not just set 
		 *	to undefined as the delete operator will do. Indicies are renumbered. NOTE: this routine delegates to splice.
		 *	@param start Numeric index of the first element to remove. Negative indices measure from the end of the array.
		 *	-1 is the last element.
		 *	@param end Numeric index of the last element to remove
		 *	@spec ejs-11
		 */
		function remove(start: Number, end: Number = -1): Void {
			if (start < 0) {
				start += length
			}
			if (end < 0) {
				end += length
			}
			splice(start, end - start + 1)
		}


		/**
		 *	Reverse the order of the objects in the array. The elements are reversed in the original array.
		 *	@return A reference to the array.
		 *	@spec ecma-3
		 */
		native function reverse(): Array 


		/**
		 *	Remove and return the first element in the array.
		 *	@returns the previous first element in the array.
		 *	@spec ecma-3
		 */
		native function shift(): Object 


		 //	TODOO - with multimethods, it would be nice to do:
		 //		native function slice(range: Range): Array 
		/**
		 *	Create a new array subset by taking a slice from an array.
		 *	@param start The array index at which to begin. Negative indicies will measure from the end so that -1 will 
		 *		specify the last element.  
		 *	@param end One beyond the index of the last element to extract. The array index at which to begin. If 
		 *		end is negative, it is measured from the end of the array.  
		 *	@param step Slice every step (nth) element. The step value may be negative.
		 *	@return A new array that is a subset of the original array.
		 *	@throws OutOfBoundsError If the start or end are equal to or greater than the length of the array, or, if the 
		 *		start is greater than the end, or, either start or end is negative.
		 *	@spec ecma-3
		 */
		native function slice(start: Number, end: Number = -1, step: Number = 1): Array 


		/**
		 *	Determine if some elements match.
		 *	Iterate over all element in the array and determine if the matching function is true for at least one element. 
		 *	This method is lazy and will cease iterating when a successful match is found.
		 *	This method is identical to the ECMA @some method. The match function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True if the match function ever is true.
		 *	@spec ecma-3
		 */
		function some(match: Function): Boolean {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					return true
				}
			}
			return false
		}


		 //	TODO: need to use Comparator instead of Function
		/**
		 *	Sort the array using the supplied compare function
		 *	@param compare Function to use to compare. A null comparator will use a text compare
		 *	@param order If order is >= 0, then an ascending order is used. Otherwise descending.
		 *	@return the sorted array reference
		 *		type Comparator = (function (*,*): AnyNumber | undefined)
		 *	@spec emca-4, ejs-11 Added the order argument.
		 */
		native function sort(compare: Function = null, order: Number = 1): Array 


		/**
		 *	Insert, remove or replace array elements. Splice modifies an array in place. 
		 *	@param start The array index at which the insertion or deleteion will begin. Negative indicies will measure 
         *	    from the end so that -1 will specify the last element.  
		 *	@param deleteCount Number of elements to delete. If omitted, splice will delete all elements from the 
         *	    start argument to the end.  
		 *	@param values The array elements to add.
		 *	@return Array containing the removed elements.
		 *	@throws OutOfBoundsError If the start is equal to or greater than the length of the array.
		 *	@spec ecma-3
		 */
		native function splice(start: Number, deleteCount: Number, ...values): Array 


		# ECMA || FUTURE
		native function toJSONString(array: Array, pretty: Boolean = false): String 


		# ECMA || FUTURE
		override native function toLocaleString(): String 


		/**
		 *	Convert the array to a single string each member of the array has toString called on it and the resulting 
		 *	strings are concatenated.
		 *	@return A string
		 *	@spec ecma-3
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Transform all elements.
		 *	Iterate over all elements in the object and transform the elements by applying the transform function. 
		 *	This modifies the object elements in-situ. The transform function is called with the following signature:
		 *		function mapper(element: Object, elementIndex: Number, arr: Array): Object
		 *	@param transform Transforming function
		 *	@spec ejs-11
		 */
		function transform(mapper: Function): Void {
			for (let i: Number in this) {
				this[i] = mapper(this[i], i, this);
			}
		}


		/**
		 *	Remove duplicate elements and make the array unique. Duplicates are detected by using "==" (ie. content 
		 *		equality, not strict equality).
		 *	@return The original array with unique elements
		 *	@spec ejs-11
		 */
		native function unique(): Array


		/**
		 *	Insert the items at the front of the array.
		 *	@param items to insert
		 *	@return Returns the array reference
		 *	@spec ecma-3
		 */
		function unshift(...items): Object {
			return insert(0, items)
		}


		/**
		 *	Array intersection. Return the elements that appear in both arrays. 
		 *	@param arr The array to join.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function & (arr: Array): Array


		/**
		 *	Append. Appends elements to an array.
		 *	@param elements The array to add append.
		 *	@return The modified original array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function << (elements: Array): Array


		/**
		 *	Array subtraction. Remove any items that appear in the supplied array.
		 *	@param arr The array to remove.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function - (arr: Array): Array


		/**
		 *	Array union. Return the union of two arrays. 
		 *	@param arr The array to join.
		 *	@return A new array
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function | (arr: Array): Array


		/**
		 *	Concatenate two arrays. 
		 *	@param arr The array to add.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function + (arr: Array): Array
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Array.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Block.es"
 */
/************************************************************************/

/*
 *	Block.es -- Block scope class used internally by the VM.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The Block type is used to represent program block scope. It is used internally and should not be 
     *	instantiated by user programs.
	 */
	native final class Block {
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Block.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Boolean.es"
 */
/************************************************************************/

/*
 *	Boolean.es -- Boolean class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Boolean class. Boolean  objects can take one of two values, true or false.
	 */
	native final class Boolean {

		/**
		 *	Boolean constructor.
		 *	@param value. Optional value to use in creating the Boolean object. If the value is omitted or 0, -1, NaN,
         *	    false, null, undefined or the empty string, then the object will be created and set to false.
         *	@spec ecma-3
		 */
		native function Boolean(...value)


	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Boolean.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/ByteArray.es"
 */
/************************************************************************/

/*
 *	ByteArray.es - ByteArray class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	ByteArrays provide a growable, integer indexed, in-memory store for bytes. ByteArrays are a powerful data 
     *	type that can be used as a simple array to store and encode data as bytes or it can be used as a Stream 
     *	implementing the Stream interface.
     *
     *	When used as a simple byte array, the ByteArray class offers a low level set of methods to insert and 
     *	extract bytes. The index operator [] can be used to access individual bytes and the copyIn and copyOut methods 
     *	can be used to get and put blocks of data. In this mode, the read and write position properties are ignored. 
     *	Accesses to the byte array are from index zero up to the size defined by the length property. When constructed, 
     *	the ByteArray can be designated as growable, in which case the initial size will grow as required to accomodate 
     *	data and the length property will be updated accordingly.
     *
     *  When used as a Stream, the byte array offers various read and write methods which store data at the location 
     *  specified by the write position property and they read data from the read position. The available method 
     *  indicates how much data is available between the read and write position pointers. The flush method will 
     *  reset the pointers to the start of the array. The length property is unchanged in behavior from when used as 
     *  a simple byte array and it specifies the overall storage capacity of the byte array. As numeric values are 
     *  read or written, they will be encoded according to the value of the endian property which can be set to 
     *  either LittleEndian or BigEndian. When used with for/in, ByteArrays will iterate or enumerate over the 
     *  available data between the read and write pointers.
     *
     *  In Stream mode ByteArrays can be configured with input and output callbacks to provide or consume data to other 
     *  streams or components. These callbacks will automatically be invoked as required when the various read/write 
     *  methods are called.
	 */
	native final class ByteArray implements Stream {

        use default namespace public

		/**
		 *	Numeric byte order constants used for the endian property
	 	 */
		static const LittleEndian: Number 	= 0
		static const BigEndian: Number 		= 1


		/**
		 *	Create a new array. This will set the default encoding.
		 *	@param size The initial size of the byte array. If not supplied a system default buffer size will be used.
         *	@param growable Set to true to automatically grow the array as required to fit written data. If growable 
         *	    is false, then some writes may return "short". ie. not be able to accomodate all written data.
		 */
		native function ByteArray(size: Number = -1, growable: Boolean = false)


		/**
		 *	Get the number of bytes that are currently available for reading from the current read position.
		 *	@returns The number of available bytes of data.
		 */
		native function get available(): Number 


		/**
		 *	Close the byte array
		 */
		function close(graceful: Boolean = false): Void
            flush()


		/**
		 *	Compress the array using zlib
		 */
		# FUTURE
		native function compress(): Void


        /**
		 *	Copy data into the array. Data is written at the $destOffset index.
		 *	@param destOffset Index in the destination byte array to copy the data to
		 *	@param src Byte array containing the data elements to copy
		 *	@param srcOffset Location in the source buffer from which to copy the data. Defaults to the start.
		 *	@param count Number of bytes to copy. Set to -1 to read all available data.
		 *	@return the number of bytes written into the array
         */
        native function copyIn(destOffset: Number, src: ByteArray, srcOffset: Number = 0, count: Number = -1): Void


		/**
		 *	Copy data from the array. Data is copied from the $srcOffset pointer.
		 *	@param dest Destination byte array
		 *	@param destOffset Location in the destination array to copy the data. Defaults to the start.
		 *	@param count Number of bytes to read. Set to -1 to read all available data.
		 *	@returns the count of bytes read. Returns 0 on end of file.
		 *	@throws IOError if an I/O error occurs.
		 */
		native function copyOut(srcOffset: Number, dest: ByteArray, destOffset: Number = 0, count: Number = -1): Number


		/**
		 *	Determine if the system is using little endian byte ordering
		 *	@return An endian encoding constant. Either LittleEndian or BigEndian
	 	 */
		native function get endian(): Number


		/**
		 *	Set the system encoding to little or big endian.
		 *	@param value Set to true for little endian encoding or false for big endian.
	 	 */
		native function set endian(value: Number): Void


		/**	
		 *	Flush the the byte array and reset the read and write position pointers. This may invoke the output callback
		 *	to send the data if the output callback is defined.
		 */
		native function flush(): Void


		/**
		 *	Get an iterator for this array to be used by "for (v in array)". This will return array indicies for 
		 *	read data in the array.
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in array)". This will return read data in the array.
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Input callback function when read data is required. The input callback should write to the supplied buffer.
		 *	@param callback Function to call to supply read data. The function is called with the following signature:
		 *		function inputCallback(buffer: ByteArray): Void
		 */
		native function set input(value: Function): Void
		//	TODO - bug. Can't have setter without a getter.
		native function get input(): Function


		/**
		 *	Get the length of an array. This is not the amount of read or write data, but is the size of the total array storage.
		 *	@return The size of the byte array.
		 */
		override native function get length(): Number


		/**
		 *	Define an output function to process (output) data. The output callback should read from the supplied buffer.
		 *	@param callback Function to invoke when the byte array is full or flush() is called.
		 *		function outputCallback(buffer: ByteArray): Number
		 */
		native function set output(callback: Function): Void
		//	TODO - bug. Can't have setter without a getter.
		native function get output(): Function


		/**
		 *	Read data from the array into another byte array. Data is read from the current read $position pointer.
		 *	@param buffer Destination byte array
		 *	@param offset Location in the destination buffer to copy the data. Defaults to the write position. If the offset is
         *	    < 0, then the write position will be updated.
		 *	@param count Number of bytes to read. Set to -1 to read all available data.
		 *	@returns the count of bytes read. Returns 0 on end of file.
		 *	@throws IOError if an I/O error occurs.
		 */
		native function read(buffer: ByteArray, offset: Number = -1, count: Number = -1): Number


		/**
		 *	Read a boolean from the array. Data is read from the current read $position pointer.
		 *	@returns a boolean
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readBoolean(): Boolean


		/**
		 *	Read a byte from the array. Data is read from the current read $position pointer.
		 *	@returns a byte
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readByte(): Number


		/**
		 *	Read a date from the array or a premature end of file. Data is read from the current read $position pointer.
		 *	@returns a date
		 *	@throws IOError if an I/O error occurs.
		 */
		native function readDate(): Date


		/**
		 *	Read a double from the array. The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a double
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readDouble(): Date


		/**
		 *	Read an 32-bit integer from the array. The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns an integer
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readInteger(): Number


		/**
		 *	Read a 64-bit long from the array.The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a long
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readLong(): Number


		/**
		 *	Return the current read position offset
		 *	@return the read offset
		 */
		native function get readPosition(): Number


		/**
		 *	Set the current read position offset
		 *	@param position The new read position
		 */
		native function set readPosition(position: Number): Void


		/**
		 *	Read a 16-bit short integer from the array.The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a short int
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readShort(): Number


		/**
		 *	Read a data from the array as a string. Read data from the read position to a string up to the write position,
         *	    but not more than count characters.
		 *	@param count of bytes to read. If -1, convert the data up to the write position.
		 *	@returns a string
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readString(count: Number = -1): String


		/**
		 *	Read an XML document from the array. Data is read from the current read $position pointer.
		 *	@returns an XML document
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readXML(): XML


        /**
         *  Reset the read and write position pointers if there is no available data.
         */
        native function reset(): Void


		/**
		 *	Get the number of data bytes that the array can store from the write position till the end of the array.
		 *	@returns The number of data bytes that can be written.
		 */
		native function get room(): Number 


		/**
		 *	Convert the data in the byte array between the read and write positions to a string.
		 *	@return A string
		 *	@spec ecma-3
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Uncompress the array using zlib
		 */
		# FUTURE
		native function uncompress(): Void


		/**
		 *	Write data to the array. Binary data is written in an optimal, platform dependent binary format. If cross-platform
         *	portability is required, use the BinaryStream to encode the data. Data is written to the current $writePosition 
         *	If the data argument is itself a ByteArray, the available data from the byte array will be copied. NOTE: the 
         *	data byte array will not have its readPosition adjusted.
		 *	@param data Data elements to write
		 *	@return the number of bytes written into the array
		 */
		native function write(...data): Number


		/**
		 *	Write a byte to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeByte(data: Number): Void


		/**
		 *	Write a short to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeShort(data: Number): Void


		/**
		 *	Write a double to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeDouble(data: Number): Void


		/**
		 *	Write a 32-bit integer to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeInteger(data: Number): Void


		/**
		 *	Write a 64 bit long integer to the array. Data is written to the current write $position pointer which is 
		 *	then incremented.
		 *	@param data Data to write
		 */
		native function writeLong(data: Number): Void


		/**
		 *	Get the current write position offset.
		 *	@return the write position
		 */
		native function get writePosition(): Number


		/**
		 *	Set the current write position offset.
		 *	@param position the new write  position
		 */
		native function set writePosition(position: Number): Void
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/ByteArray.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Date.es"
 */
/************************************************************************/

/*
 *	Date.es -- Date class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	General purpose class for representing and working with dates, times, time spans, time zones and locales.
	 */
	native final class Date {

        use default namespace public

		/**
		 *	Construct a new date object. Permissible constructor forms:
         *	<ul>
		 *		<li>Date()</li>
		 *		<li>Date(milliseconds)</li>
		 *		<li>Date(dateString)</li>
		 *		<li>Date(year, month, date)</li>
		 *		<li>Date(year, month, date, hour, minute, second)</li>
         *  </ul>
		 */
        native function Date(...args)


		/**
		 *	Return the day of the week in local time.
		 *	@return The integer day of the week (0 - 6, where 0 is Sunday)
		 */
		native function get day(): Number 

//  TODO - why are we missing the setters?

		/**
		 *	Return the day of the year in local time.
		 *	@return The integer day of the year (0 - 366)
		 *	@spec ejs-11
		 */
		native function get dayOfYear(): Number 


		 //	TODO not a great name but matches getDate()
		/**
		 *	Return the day of the month.
		 *	@return Returns the day of the year (1-31)
		 */
		native function get date(): Number 


		/**
		 *	Time in milliseconds since the date object was constructed
		 */
		native function get elapsed(): Number


		/**
		 *	Return the year as four digits.
		 *	@return The year
		 */
		native function get fullYear(): Number 


		/**
		 *	Set the year as four digits according to the locale
		 *	@param year Year to set.
		 */
		native function set fullYear(year: Number): void


		/**
		 *	Return a formatted date string of the date. This corresponds to the C language strftime conventionts. 
		 *	The format specifiers are:
		 *
		 *	<ul>
		 *	<li>%A    national representation of the full weekday name.</li>
		 *	<li>%a    national representation of the abbreviated weekday name.</li>
		 *	<li>%B    national representation of the full month name.</li>
		 *	<li>%b    national representation of the abbreviated month name.</li>
		 *	<li>%C    (year / 100) as decimal number; single digits are preceded by a zero.</li>
		 *	<li>%c    national representation of time and date.</li>
		 *	<li>%D    is equivalent to ``%m/%d/%y''.</li>
		 *	<li>%d    the day of the month as a decimal number (01-31).</li>
		 *	<li>%E* %O* POSIX locale extensions.  The sequences %Ec %EC %Ex %EX %Ey %EY %Od %Oe %OH %OI %Om %OM %OS %Ou
		 *		   %OU %OV %Ow %OW %Oy are supposed to provide alternate representations. Additionly %OB implemented
		 *		   to represent alternative months names (used standalone,
		 *		   without day mentioned).</li>
		 *	<li>%e    the day of month as a decimal number (1-31); single digits are preceded by a blank.</li>
		 *	<li>%F    is equivalent to ``%Y-%m-%d''.</li>
		 *	<li>%G    a year as a decimal number with century.  This year is the one that contains the greater part of
		 *	       the week (Monday as the first day of the week).</li>
		 *	<li>%g    the same year as in ``%G'', but as a decimal number without century (00-99).</li>
		 *	<li>%H    the hour (24-hour clock) as a decimal number (00-23).</li>
		 *	<li>%h    the same as %b.</li>
		 *	<li>%I    the hour (12-hour clock) as a decimal number (01-12).</li>
		 *	<li>%j    the day of the year as a decimal number (001-366).</li>
		 *	<li>%k    the hour (24-hour clock) as a decimal number (0-23); single digits are preceded by a blank.</li>
		 *	<li>%l    the hour (12-hour clock) as a decimal number (1-12); single digits are preceded by a blank.</li>
		 *	<li>%M    the minute as a decimal number (00-59).</li>
		 *	<li>%m    the month as a decimal number (01-12).</li>
		 *	<li>%m    the month as a decimal number (01-12).</li>
		 *	<li>%n    a newline.</li>
		 *	<li>%O*   the same as %E*.</li>
		 *	<li>%p    national representation of either "ante meridiem" or "post meridiem" as appropriate.</li>
		 *	<li>%R    is equivalent to ``%H:%M''.</li>
		 *	<li>%r    is equivalent to ``%I:%M:%S %p''.</li>
		 *	<li>%S    the second as a decimal number (00-60).</li>
		 *	<li>%s    the number of seconds since the Epoch, UTC (see mktime(3)).</li>
		 *	<li>%T    is equivalent to ``%H:%M:%S''.</li>
		 *	<li>%t    a tab.</li>
		 *	<li>%U    the week number of the year (Sunday as the first day of the week) as a decimal number (00-53).</li>
		 *	<li>%u    the weekday (Monday as the first day of the week) as a decimal number (1-7).</li>
		 *	<li>%V    the week number of the year (Monday as the first day of the week) as a decimal
		 *		   number (01-53).  If the week containing January 1 has four or more days in the new year, then it
		 *		   is week 1; otherwise it is the last week of the previous year, and the next week is week 1.</li>
		 *	<li>%v    is equivalent to ``%e-%b-%Y''.</li>
		 *	<li>%W    the week number of the year (Monday as the first day of the week) as a decimal number (00-53).</li>
		 *	<li>%w    the weekday (Sunday as the first day of the week) as a decimal number (0-6).</li>
		 *	<li>%X    national representation of the time.</li>
		 *	<li>%x    national representation of the date.</li>
		 *	<li>%Y    the year with century as a decimal number.</li>
		 *	<li>%y    the year without century as a decimal number (00-99).</li>
		 *	<li>%Z    the time zone name.</li>
		 *	<li>%z    the time zone offset from UTC; a leading plus sign stands for east of UTC, a minus
		 *		   sign for west of UTC, hours and minutes follow with two digits each and no delimiter between them
		 *		   (common form for RFC 822 date headers).</li>
		 *	<li>%+    national representation of the date and time (the format is similar to that produced by date(1)).</li>
		 *	<li>%%    Literal percent.</li>
		 *	</ul>
		 *
		 *	@param layout Format layout.
		 *	@return string representation of the date.
		 *	@spec ejs-11
		 */
		native function format(layout: String): String 


		/**
		 *	Return the day of the week in local time.
		 *	@return The integer day of the week (0 - 6, where 0 is Sunday)
		 */
		# ECMA
		native function getDay(): Number 


		/**
		 *	Return the day of the month.
		 *	@return Returns the day of the year (1-31)
		 */
		# ECMA
		native function getDate(): Number 


		/**
		 *	Return the year as four digits.
		 *	@return The integer year
		 */
		# ECMA
		native function getFullYear(): Number 


		/**
		 *	Return the hour (0 - 23) in local time.
		 *	@return The integer hour of the day
		 */
		# ECMA
		native function getHours(): Number 


		/**
		 *	Return the millisecond (0 - 999) in local time.
		 *	@return The number of milliseconds as an integer
		 */
		# ECMA
		native function getMilliseconds(): Number 


		/**
		 *	Return the minute (0 - 59) in local time.
		 *	@return The number of minutes as an integer
		 */
		# ECMA
		native function getMinutes(): Number 


		/**
		 *	Return the month (1 - 12) in local time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function getMonth(): Number 


		/**
		 *	Return the second (0 - 59) in local time.
		 *	@return The number of seconds as an integer
		 */
		# ECMA
		native function getSeconds(): Number 


		/**
		 *	Return the number of milliseconds since midnight, January 1st, 1970.
		 *	@return The number of milliseconds as a long
		 */
		# ECMA
		static function getTime(): Number {
			return time
		}


		/**
		 *	Return the month (1 - 12) in UTC time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function getUTCMonth(): Number 


		/**
		 *	Return the number of minutes between the local computer time and Coordinated Universal Time.
		 *	@return The number of minutes as an integer
		 */
		# ECMA
		native function getTimezoneOffset(): Number


		/**
		 *	Return the current hour (0 - 23) in local time.
		 *	@return The integer hour of the day
         *	TODO - should this be hour? for consistency with day
		 */
		native function get hours(): Number 


		/**
		 *	Set the current hour (0 - 59) according to the locale
		 *	@param The hour as an integer
		 */
		native function set hours(hour: Number): void


		/**
		 *	Return the current millisecond (0 - 999) in local time.
		 *	@return The number of milliseconds as an integer
		 */
		native function get milliseconds(): Number 


		/**
		 *	Set the current millisecond (0 - 999) according to the locale
		 *	@param The millisecond as an integer
		 */
		native function set milliseconds(ms: Number): void


		/**
		 *	Return the current minute (0 - 59) in local time.
		 *	@return The number of minutes as an integer
		 */
		native function get minutes(): Number 


		/**
		 *	Set the current minute (0 - 59) according to the locale
		 *	@param The minute as an integer
		 */
		native function set minutes(min: Number): void


		/**
		 *	Return the current month (0 - 11) in local time.
		 *	@return The month number as an integer
		 */
		native function get month(): Number 


		/**
		 *	Set the current month (0 - 11) according to the locale
		 *	@param The month as an integer
		 */
		native function set month(month: Number): void



		/**
		 *	Time in nanoseconds since the date object was constructed
		 */
        # ECMA
		function nanoAge(): Number {
            return elapsed() * 1000
        }


		//    TODO - could take an arg for N days forward or backward
		/**
		 *	Return a new Date object that is one day greater than this one.
		 *  @param inc Increment in days to add (or subtract if negative)
		 *	@return A Date object
		 *	@spec ejs-11
		 */
		native function nextDay(inc: Number = 1): Date


		/**
		 *	Return the current time as milliseconds since Jan 1 1970.
		 */
		static native function now(): Number


		/**
		 *	Return a new Date object by parsing the argument string.
		 *	@param arg The string to parse
		 *	@param defaultDate Default date to use to fill out missing items in the date string.
		 *	@return Return a new Date.
		 *	@spec ejs-11
		 */
		static native function parseDate(arg: String, defaultDate: Date = undefined): Date


		/**
		 *	Return a new Date object by parsing the argument string.
		 *	@param arg The string to parse
		 *	@param defaultDate Default date to use to fill out missing items in the date string.
		 *	@return Return a new date number.
		 */
		# ECMA
		static function parse(arg: String, defaultDate: Number = undefined): Number {
            var d: Date = parseDate(arg, defaultDate)
            return d.time
        }


		/**
		 *	Return the current second (0 - 59) in local time.
		 *	@return The number of seconds as an integer
		 */
		native function get seconds(): Number 


		/**
		 *	Set the current second (0 - 59) according to the locale
		 *	@param The second as an integer
		 */
		native function set seconds(sec: Number): void


		/**
		 *	Set the current year as four digits according to the locale
		 */
		# ECMA
		native function setFullYear(year: Number): void


		/**
		 *	Set the current hour (0 - 59) according to the locale
		 *	@param The hour as an integer
		 */
		# ECMA
		native function setHours(hour: Number): void


		/**
		 *	Set the current millisecond (0 - 999) according to the locale
		 *	@param The millisecond as an integer
		 */
		# ECMA
		native function setMilliseconds(ms: Number): void


		/**
		 *	Set the current minute (0 - 59) according to the locale
		 *	@param The minute as an integer
		 */
		# ECMA
		native function setMinutes(min: Number): void


		/**
		 *	Set the current month (0 - 11) according to the locale
		 *	@param The month as an integer
		 */
		# ECMA
		native function setMonth(month: Number): void


		/**
		 *	Set the current second (0 - 59) according to the locale
		 *	@param The second as an integer
		 */
		# ECMA
		native function setSeconds(sec: Number, milli: Number = getMilliseconds()): void


		/**
		 *	Set the number of milliseconds since midnight, January 1st, 1970.
		 *	@param The millisecond as a long
		 */
		# ECMA
		native function setTime(ms: Number): void


		/**
		 *	Return the number of milliseconds since midnight, January 1st, 1970 and the current date object.
		 *	@return The number of milliseconds as a long
		 */
		native static function get time(): Number 


		/**
		 *	Set the number of milliseconds since midnight, January 1st, 1970.
		 *	@return The number of milliseconds as a long
		 */
		native static function set time(value: Number): Number 


		/**
		 *	Return a localized string containing the date portion excluding the time portion of the date according to
		 *	the specified locale.
		 *	Sample format: "Fri, 15 Dec 2006 GMT-0800"
		 *	@return A string representing the date portion.
		 */
		# ECMA
		native function toDateString(locale: String = null): String 


		/**
		 *	Return an ISO formatted date string.
		 *	Sample format: "2006-12-15T23:45:09.33-08:00"
		 *	@return An ISO formatted string representing the date.
		 */
		# ECMA
		native function toISOString(): String 


		/**
		 *	Return a JSON encoded date string.
		 *	@return An JSON formatted string representing the date.
		 */
		# ECMA
		native function toJSONString(pretty: Boolean = false): String 


		/**
		 *	Return a localized string containing the date portion excluding the time portion of the date according to
		 *	the current locale.
		 *	Sample format: "Fri, 15 Dec 2006 GMT-0800"
		 *	@return A string representing the date portion.
		 */
		# ECMA
		native function toLocaleDateString(): String 


		/**
		 *	Return a localized string containing the date.
		 *	Sample format: "Fri, 15 Dec 2006 23:45:09 GMT-0800"
		 *	@return A string representing the date.
		 */
		# ECMA
		override native function toLocaleString(): String 


		/**
		 *	Return a string containing the time portion of the date according to the current locale.
		 *	Sample format: "23:45:09 GMT-0800"
		 *	@return A string representing the time.
		 */
		# ECMA
		native function toLocaleTimeString(): String 


		/**
		 *	Return a string containing the date according to the locale.
		 *	Sample format: "Fri, 15 Dec 2006 23:45:09 GMT-0800"
		 *	@return A string representing the date.
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Return a string containing the time portion of the date according to the locale.
		 *	Sample format: "23:45:09 GMT-0800"
		 *	@return A string representing the time.
		 */
		# ECMA
		native function toTimeString(): String 


		/**
		 *	Return a string containing the date according to the locale.
		 *	Sample format: "Sat, 16 Dec 2006 08:06:21 GMT"
		 *	@return A string representing the date.
		 */
		# ECMA
		native function toUTCString(): String 


		/**
		 *	Construct a new date object interpreting its arguments in UTC rather than local time.
		 *	@param year Year
		 *	@param month Month of year
		 *	@param day Day of month
		 *	@param hours Hour of day
		 *	@param minutes Minute of hour
		 *	@param seconds Secods of minute
		 *	@param milliseconds Milliseconds of second
		 */
		# ECMA
        native function UTC(year: Number, month: Number, day: Number = NOARG, hours: Number = NOARG, 
			minutes: Number = NOARG, seconds: Number = NOARG, milliseconds: Number = NOARG): Date


//  TODO - should this be 0-11?
		/**
		 *	Return the current month (1 - 12) in UTC time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function get UTCmonth(): Number 


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 */ 
		# ECMA
		override function valueOf(): String {
			return getTime()
		}

		/**
		 *	Return the current year as two digits.
		 *	@return The integer year
		 */
		native function get year(): Number 


		/**
		 *	Set the current year as two digits according to the locale
		 *	@param year Year to set.
		 */
		native function set year(year: Number): void



		/**
		 *	Date difference. Return a new Date that is the difference of the two dates.
		 *	@param The operand date
		 *	@return Return a new Date.
		 */
		# TODO
		native function -(date: Date): Date


/*	TODO - ECMA needs these

		Would be nice to be able to do:

		var d: Date 
		d.locale = "en_us"
		d.locale = "UTC"
		when = d.month

		native function getUTCDate(): String
		native function getUTCFullYear(): String
		native function getUTCMonth(): String
		native function getUTCDay(): String
		native function getUTCHours(): String
		native function getUTCMinutes(): String
		native function getUTCSeconds(): String
		native function getUTCMilliSeconds(): String
		native function setUTCDate(): void
		native function setUTCFullYear(): void
		native function setUTCMonth(): void
		native function setUTCDay(): void
		native function setUTCHours(): void
		native function setUTCMinutes(): void
		native function setUTCSeconds(): void
		native function setUTCMilliSeconds(): void
*/

	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Date.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Error.es"
 */
/************************************************************************/

/*
 *	Error.es -- Error exception classes
 *
 *	Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Arguments error exception class. Thrown when there are too few or too many arguments or the arguments 
	 *	cannot be cast to the required type.
	 *	@spec ejs-11
	 */
	native dynamic class ArgError extends Error {

        /**
         *  ArgError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function ArgError(message: String = null) 
	}


	/**
	 *	Arithmetic error exception class. Thrown when the system cannot perform an arithmetic operation, 
	 *	e.g. on divide by zero.
	 *	@spec ejs-11
	 */
	native dynamic class ArithmeticError extends Error {

        /**
         *  ArithmeticError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function ArithmeticError(message: String = null) 
	}


	/**
	 *	Assertion error exception class. Thrown when an assertion fails.
	 *	@spec ejs-11
	 */
	native dynamic class AssertError extends Error {

        /**
         *  AssertError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function AssertError(message: String = null) 
	}


	/**
	 *	Code (instruction) error exception class. Thrown when an illegal or insecure operation code is detected 
	 *	in the instruction stream.
	 *	@spec ejs-11
	 */
	native dynamic class InstructionError extends Error {

        /**
         *  InstructionError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function InstructionError(message: String = null) 
	}


	/**
	 *	Base class for error exception objects. Exception objects are created by the system as part of changing 
	 *	the normal flow of execution when some error condition occurs. 
	 *
	 *	When an exception is created and acted upon ("thrown"), the system transfers the flow of control to a 
	 *	pre-defined instruction stream (the handler or "catch" code). The handler may return processing to the 
	 *	point at which the exception was thrown or not. It may re-throw the exception or pass control up the call stack.
	 */
	native dynamic class Error {
		/**
		 *	Exception error message.
		 */
		native var message: String


		/**
		 *	Get an optional error code
         *	@return any defined error code
		 */
		native function get code(): Number


		/**
		 *	Set an optional error code
         *	@param value Error code to set
		 */
		native function set code(value: Number): Void


		/**
		 *	Execution stack backtrace. Contains the execution stack backtrace at the time the exception was thrown.  
		 */
		native var stack: String 

        /**
         *  Construct a new Error object.
         *  @params message Message to use when defining the Error.message property
         */
		native function Error(message: String = null)
	}


	/**
	 *	IO error exception class. Thrown when an I/O/ interruption or failure occurs, e.g. a file is not found 
	 *	or there is an error in a communication stack.
	 *	@spec ejs-11
	 */
	native dynamic class IOError extends Error {

        /**
         *  IOError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function IOError(message: String = null) 
	}


	/**
	 *	Internal error exception class. Thrown when some error occurs in the virtual machine.
	 *	@spec ejs-11
	 */
	native dynamic class InternalError extends Error {

        /**
         *  InternalError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function InternalError(message: String = null) 
	}


	/**
	 *	Memory error exception class. Thrown when the system attempts to allocate memory and none is available 
	 *	or the stack overflows.
	 *	@spec ejs-11
	 */
	native dynamic class MemoryError extends Error {

        /**
         *  MemoryError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function MemoryError(message: String = null) 
	}


	/**
	 *	OutOfBounds error exception class. Thrown to indicate that an attempt has been made to set or access an 
	 *	object's property outside of the permitted set of values for that property. For example, an array has been 
	 *	accessed with an illegal index or, in a date object, attempting to set the day of the week to greater then 7.
	 *	@spec ejs-11
	 */
	native dynamic class OutOfBoundsError extends Error {

        /**
         *  OutOfBoundsError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function OutOfBoundsError(message: String = null) 
	}


	/**
	 *	Reference error exception class. Thrown when an invalid reference to an object is made, e.g. a method is 
	 *	invoked on an object whose type does not define that method.
	 */
	native dynamic class ReferenceError extends Error {

        /**
         *  ReferenceError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function ReferenceError(message: String = null)
	}


	/**
	 *	Resource error exception class. Thrown when the system cannot allocate a resource it needs to continue, 
	 *	e.g. a native thread, process, file handle or the like.
	 *	@spec ejs-11
	 */
	native dynamic class ResourceError extends Error {

        /**
         *  ResourceError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function ResourceError(message: String = null) 
	}


	/**
	 *	Security error exception class. Thrown when an access violation occurs. Access violations include attempting 
	 *	to write a file without having write permission or assigning permissions without being the owner of the 
	 *	securable entity.
	 *	@spec ejs-11
	 */
    # FUTURE
	native dynamic class SecurityError extends Error {

        /**
         *  SecurityError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function SecurityError(message: String = null) 
	}


	/**
	 *	State error exception class. Thrown when an object cannot be transitioned from its current state to the 
	 *	desired state, e.g. calling "sleep" on an interrupted thread.
	 *	@spec ejs-11
	 */
	native dynamic class StateError extends Error {

        /**
         *  StateError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function StateError(message: String = null) 
	}


	/**
	 *	Syntax error exception class. Thrown when the system cannot parse a character sequence for the intended 
	 *	purpose, e.g. a regular expression containing invalid characters.
	 */
	native dynamic class SyntaxError extends Error {

        /**
         *  SyntaxError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function SyntaxError(message: String = null) 
	}


	/**
	 *	Type error exception class. Thrown when a type casting or creation operation fails, e.g. when an operand 
	 *	cannot be cast to a type that allows completion of a statement or when a type cannot be found for object 
	 *	creation or when an object cannot be instantiated given the values passed into "new".
	 */
	native dynamic class TypeError extends Error {

        /**
         *  TypeError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function TypeError(message: String = null) 
	}


	/**
	 *	URI error exception class. Thrown a URI fails to parse.
	 */
	native dynamic class URIError extends Error {

        /**
         *  URIError constructor.
         *  @params message Message to use when defining the Error.message property
         */
		native function URIError(message: String = null) 
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2009-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 2009-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 *
 */
/************************************************************************/
/*
 *  End of file "../es/core/Error.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Function.es"
 */
/************************************************************************/

/*
 *	Function.es -- Function class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The Function type is used to represent closures, function expressions and class methods. It contains a 
	 *	reference to the code to execute, the execution scope and possibly a bound "this" reference.
	 */
	native final class Function {

        use default namespace public

		/**
		 *	Invoke the function on another object.
		 *	@param thisObject The object to set as the "this" object when the function is called.
		 *	@param args Array of actual parameters to the function.
		 *	@return Any object returned as a result of applying the function
		 *	@throws ReferenceError If the function cannot be applied to this object.
		 */
		native function apply(thisObject: Object, args: Array): Object 

		/**
		 *	Invoke the function on another object.
		 *	@param thisObject The object to set as the "this" object when the function is called.
		 *	@param args Array of actual parameters to the function.
		 *	@return Any object returned as a result of applying the function
		 *	@throws ReferenceError If the function cannot be applied to this object.
		 */
		native function call(thisObject: Object, ...args): Object 

		/**
		 *	Return the bound object representing the "this" object. Functions carry both a lexical scoping and 
		 *	the owning "this" object.
		 *	@return An object
		 */
        # FUTURE
		native function get boundThis(): Object

		//	TODO - ecma has more than this
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Function.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Global.es"
 */
/************************************************************************/

/*
 *	Global.es -- Global variables, namespaces and functions.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
	TODO missing:

	Classes: Vector, Map, EncodingError
	Types: EnumerableId, Numeric, Strings, Booleans, FieldIterator, FieldValueIterator, TypeIterator, InterfaceIterator
	Interfaces: Field, FieldValue, Type, NominalType, InterfaceType, ClassType, AnyType, NullType, UndefinedType,
		UnionType, RecordType, FunctionType, ArrayType
	Functions: get(), set()

	function isIntegral(n: Number): Boolean
	function copySign(x: Number, y:Number): Number
	function sign(n: Number): Number					// Return -1 for <0,  1 for >= 0 for NaN
	function isInt(n: Number): Boolean					// integral & 32 bits
	function isUint(n: Number): Boolean					// integral & 32 bits and +ve
	function toInt(n: Number): Number
	function toUint(n: Number): Number
    function get(obj:Object!, name:string) : *;
    function set(obj:Object!, name:string, val:*) : void;
    function eval(cmd: String)
 */

/*
 *	Notes:
 *		native means supplied as a builtin (native C/Java)
 *		intrinsic implies ReadOnly, DontDelete, DontEnum
 */

module ejs {


    /**
     *  The public namespace used to make entities visible accross modules.
     */
	public namespace public

    /**
     *  The internal namespace used to make entities visible within a single module only.
     */
	public namespace internal

    /**
     *  The intrinsic namespace used for entities that are part of and intrinsic to, the Ejscript platform.
     */
	public namespace intrinsic

    /**
     *  The iterator namespace used to defined iterators.
     */
	public namespace iterator

    /**
     *  The CONFIG namespace used to defined conditional compilation directives.
     */
	public namespace CONFIG

	use default namespace intrinsic
	use namespace iterator

	use namespace "ejs.sys"

    //  TODO - refactor and reduce these
	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const TODO: Boolean = false

	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const FUTURE: Boolean = false

	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const ASC: Boolean = false

	/** 
     *  Conditional compilation constant. Used to enable the compilation of elements only for creating the API documentation.
     */  
	const DOC_ONLY: Boolean = false

	/** 
     *  Conditional compilation constant. Used to deprecate elements.
     */  
	const DEPRECATED: Boolean = false

    //  TODO - remove. Should be using Config.RegularExpressions
	/** 
     *  Conditional compilation constant. Used to deprecate elements.
     */  
	const REGEXP: Boolean = true

	/*
		FUTURE - ECMA

		type AnyNumber = (Number);
		type AnyBoolean = Boolean
		type AnyType = Boolean
		type AnyString = String
		type FloatNumber = (Number)
        var AnyNumber: Type = Number
	 */


	/**
	 *	Alias for the Boolean type
	 *	@spec ecma4
	 */
	native const boolean: Type = Boolean


	/**
	 *	Alias for the Number type
	 *	@spec ecma4
	 */
	native const double: Type = Number


	/**
	 *	Alias for the Number type
	 *	@spec ejs-11
	 */
	native const num: Type = Number


	/**
	 *	Alias for the String type
	 *	@spec ecma4
	 */
	native const string: Type = String


	/**
	 *	Boolean false value.
 	 */
	native const false: Boolean


	/**
	 *	Global variable space reference. The global variable references an object which is the global variable space. 
     *	This is useful when guaranteed access to a global variable is required. e.g. global.someName.
 	 */
	native var global: Object


	/**
	 *	Null value. The null value is returned when testing a nullable variable that has not yet had a 
	 *	value assigned or one that has had null explicitly assigned.
	 */
	native const null: Null


	/**
	 *	Infinity value.
 	 */
	native const Infinity: Number


	/**
	 *	Negative infinity value.
 	 */
	native const NegativeInfinity: Number


	/**
	 *	Invalid numeric value. If the numeric type is set to an integral type, the value is zero.
	 */
	native const NaN: Number


	/**
	 *	StopIteration used by iterators
	 */
    iterator native final class StopIteration {}


	/**
	 *	True value.
	 */
	native const true: Boolean


	/**
	 *	Undefined variable value. The undefined value is returned when testing
	 *	for a property that has not been defined. 
	 */
	native const undefined: Void


	/**
	 *	void type value. 
	 *	This is an alias for Void.
	 *	@spec ejs-11
	 */
	native const void: Type = Void


	/**
	 *	Assert a condition is true. This call tests if a condition is true by testing to see if the supplied 
	 *	expression is true. If the expression is false, the interpreter will throw an exception.
	 *	@param condition JavaScript expression evaluating or castable to a Boolean result.
	 *	@return True if the condition is.
	 *	@spec ejs-11
	 */
	native function assert(condition: Boolean): Boolean


    /*
     *  Convenient way to trap to the debugger
     */
    native function breakpoint(): Void


    /**
     *  Replace the base type of a type with an exact clone. 
     *  @param klass Class in which to replace the base class.
     *  @spec ejs-11
     */
	native function cloneBase(klass: Type): Void


	/**
	 *	Convert a string into an object. This will parse a string that has been encoded via serialize. It may contain
	 *	nested objects and arrays. This is a static method.
	 *	@param name The string containing the object encoding.
	 *	@return The fully constructed object or undefined if it could not be reconstructed.
	 *	@throws IOError If the object could not be reconstructed from the string.
	 *	@spec ejs-11
	 */
	native function deserialize(obj: String): Object


	# ECMA || FUTURE
	native function decodeURI(str: String): String


	# ECMA || FUTURE
	native function decodeURIComponent(str: String): String


    /**
     *  Dump the contents of objects. Used for debugging, this routine serializes the objects and prints to the standard
     *  output.
     *  @param args Variable number of arguments of any type
     */
	function dump(...args): Void {
		for each (var e: Object in args) {
			print(serialize(e))
		}
	}


	/**
	 *	Print the arguments to the standard error with a new line appended. This call evaluates the arguments, 
	 *	converts the result to strings and prints the result to the standard error. Arguments are converted to 
	 *	strings using the normal JavaScript conversion rules. Objects will have their @toString function called 
	 *	to get a string equivalent of their value. This output is currently vectored to the application log.
	 *	If the log is redirected to a log file, then print output is also.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function eprint(...args): void


	# ECMA || FUTURE
	native function escape(str: String): String


	# ECMA || FUTURE
	native function encodeURI(str: String): String


	# ECMA || FUTURE
	native function encodeURIComponent(str: String): String


	/**
	 *	Evaluate a script. Only present in the compiler executable: ec.
	 *	@param script Script to evaluate
	 *	@returns the the script expression value.
	 */
	# FUTURE
	native function eval(script: String): Object


	//	TODO - move this to System/App/Debug and use "platform" (internal use only) namespace
	/**
	 *	Format the current call stack. Used for debugging and when creating exception objects.
	 *	@spec ejs-11
	 */
	native function formatStack(): String


    /**
     *	Get the object's Unique hash id. All objects have a unique object hash. 
     *	@return This property accessor returns a long containing the object's unique hash identifier. 
     *	@spec ecma-3
     */ 
    native function hashcode(o: Object): Number


	# ECMA || FUTURE
	native function isNaN(str: String): Number


	# ECMA || FUTURE
	native function isFinite(str: String): Number


    /**
     *  Load a script or module
     *  @param file path name to load. File will be interpreted relative to EJSPATH if it is not found as an absolute or
     *      relative file name.
     */
	native function load(file: String): Void


	/**
	 *	Print the arguments to the standard output with a new line appended. This call evaluates the arguments, 
	 *	converts the result to strings and prints the result to the standard output. Arguments are converted to 
	 *	strings using the normal JavaScript conversion rules. Objects will have their @toString function called 
	 *	to get a string equivalent of their value. This output is currently vectored to the application log.
	 *	If the log is redirected to a log file, then print output is also.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function print(...args): void


	//	TODO - Config.Debug is failing:  # Config.Debug
	/**
	 *	Print variables for debugging.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function printv(...args): void


	/**
	 *	Parse a string and convert to a primitive type
	 */
	native function parse(input: String, preferredType: Type = null): Object


	//	TODO - need to document these
	# ECMA || FUTURE
	native function parseInt(str: String, radix: Number = 10): Number


	# ECMA || FUTURE
	native function parseFloat(str: String): Number


	/**
	 *	Encode an object as a string. This function returns a literal string for the object and all its properties. 
	 *	If @maxDepth is sufficiently large (or zero for infinite depth), each property will be processed recursively 
	 *	until all properties are rendered. 
	 *	@param maxDepth The depth to recurse when converting properties to literals. If set to zero, the depth is infinite.
	 *	@param all Encode non-enumerable and class fixture properties and functions.
	 *	@param base Encode base class properties.
	 *	@return This function returns an object literal that can be used to reinstantiate an object.
	 *	@throws TypeError If the object could not be converted to a string.
	 *	@spec ejs-11
	 */ 
	native function serialize(obj: Object, maxDepth: Number = 0, all: Boolean = false, base: Boolean = false): String

	/*
     *  TODO - remove
	 *	Determine the type of a variable. 
	 *	@param o Variable to examine.
	 *	@return Returns a string containing the arguments type. Possible types are:
	 *		@li $undefined "undefined"
	 *		@li $Object "object"
	 *		@li $Boolean "boolean"
	 *		@li $Function "function"
	 *		@li Number "number"
	 *		@li String "string"
	 *	@remarks Note that lower case names are returned for class names.
	 *	@spec ejs-11
	native function typeof(o: Object): String
	 */


    //  TODO - temp only
    function printHash(name: String, o: Object): Void {
        print("%20s %X" % [name, hashcode(o)])
    }
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Global.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Iterator.es"
 */
/************************************************************************/

/**
 *	Iterator.es -- Iteration support via the Iterable interface and Iterator class. 
 *
 *	This provides a high performance native iterator for native classes. 
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	/**
	 *	Iterable is a temporary interface until we have the IterableType structural type.
	 */
	iterator interface Iterable {
        use default namespace iterator

		/**
		 *	Get an Iterator.
		 *	@param deep Iterate over prototype properties when the Iterator calls next()
		 *	@return An Iterator
		 */
		function get(deep: Boolean = false): Iterator
		function getValues(deep: Boolean = false): Iterator
	}


	/**
	 *	Iterator is a helper class to implement iterators.
	 */
	iterator native final class Iterator {

        use default namespace public

		/*
         *  UNUSED
		 *	Current element index
		native var index: Object

		 *	Object to iterate over
		native var obj: Object
		 */

		/*
         *  UNUSED
		 *	Construct a new Iterator
		 *	@param getNextElement Function invoked by next() to return the next element to iterate
		 *	@param obj Target object to iterate. May be null if getNextElement is a bound method
         *  native function Iterator(obj: Object, getNextElement: Function)
		 */

		//	TODO - not sure about this namespace. Object literals are "" and code gen uses EMPTY_NAMESPACE
		/**
		 *	Return the next element in the object.
		 *	@return An object
		 *	@throws StopIteration
		 */
		native function next(): Object
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Iterator.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/JSON.es"
 */
/************************************************************************/

/*
 *	JSON.es -- JSON class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	final class JSON {

        use default namespace public

		/**
         *  Parse a string JSON representation and return an object equivalent
         *  @param data JSON string data to parse
         *  @param filter The optional filter parameter is a function that can filter and transform the results. It 
         *      receives each of the keys and values, and its return value is used instead of the original value. If 
         *      it returns what it received, then the structure is not modified. If it returns undefined then the 
         *      member is deleted. NOTE: the filter function is not supported
         *  @return An object representing the JSON string.
		 */
		static function parse(data: String, filter: Function = null): Object {
            return deserialize(data)
        }

		/**
         *  Convert an object into a string JSON representation
         *  @param obj Object to stringify
         *  @param replacer an optional parameter that determines how object values are stringified for objects without a 
         *      toJSON method. It can be a function or an array.
         *  @param indent an optional parameter that specifies the indentation of nested structures. If it is omitted, 
         *      the text will be packed without extra whitespace. If it is a number, it will specify the number of spaces 
         *      to indent at each level. If it is a string (such as '\t' or '&nbsp;'), it contains the characters used to 
         *      indent at each level.
         *  @return A JSON string representing the object
		 */
		static function stringify(obj: Object, replacer: Object = null, indent: Number = 0): String {
            return serialize(obj)
        }
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/JSON.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Math.es"
 */
/************************************************************************/

/*
 *	Math.es -- Math class 
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The Math class provides a set of static methods for performing common arithmetic, exponential and 
	 *	trigonometric functions. It also provides commonly used constants such as PI. See also the Number class.
	 *	Depending on the method and the supplied argument, return values may be real numbers, NaN (not a number) 
	 *	or positive or negative infinity.
	 */
    # FUTURE
	class Math extends Object 
	{

		/**
		 *	The ratio of the circumference to the diameter of a circle.
		 */
		native static const PI: Number = 3.141592653589793


		/**
		 *	Base of natural logarithms (Euler's number).
		 */
		native static const E: Number = 2.718281828459045


		/**
		 *	Natural log of 10.
		 */
		native static const LN10: Number = 2.302585092994046


		/**
		 *	Natural log of 2.
		 */
		native static const LN2: Number = 0.6931471805599453


		/**
		 *	Base 2 log of e.
		 */
		native static const LOG2E: Number = 1.4426950408889634


		/**
		 *	Base 10 log of e.
		 */
		native static const LOG10E: Number = 0.4342944819032518


		/**
		 *	Reciprocal of the square root of 2.
		 */
		native static const SQRT1_2: Number = 0.7071067811865476


		/**
		 *	Square root of 2.
		 */
		native static const SQRT2: Number = 1.4142135623730951


        //  TODO - doc
        native static function abs(value: Number): FloatNumber 

		/**
		 *	Calculates the arc cosine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The arc cosine of the argument 
		 */
		native static function acos(angle: Number): FloatNumber 
		

		/**
		 *	Calculates the arc sine of an angle (in	radians).
		 *	@param oper The operand.
		 *	@return The arc sine of the argument 
		 */
		native static function asin(oper: Number): FloatNumber 
		

		/**
		 *	Calculates the arc tangent of an angle (in radians).
		 *	@param oper The operand.
		 *	@return The arc tanget of the argument 
		 */
		native static function atan(oper: Number): FloatNumber 
		

		//	TODO - what does this fn really do
		/**
		 *	Calculates the arc tangent of an angle (in radians).
		 *	@param x the x operand.
		 *	@param y the y operand.
		 *	@return The arc tanget of the argument 
		 */
		native static function atan2(y: Number, x: Number): FloatNumber 
		

		/**
		 *	Return the smallest integer greater then this number.
		 *	@return The ceiling
		 */
		native function ceil(oper: Number): Number


		/**
		 *	Calculates the cosine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The cosine of the argument 
		 */
		native static function cos(angle: Number): FloatNumber 
		
        native static function exp(angle: Number): FloatNumber 

		/**
		 *	Returns the largest integer smaller then the argument.
		 *	@param oper The operand.
		 *	@return The floor
		 */
		native function floor(oper: Number): Number


		/**
		 *	Calculates the natural log (ln) of a number.
		 *	@param oper The operand.
		 *	@return The natural log of the argument
		 */
		native static function ln(oper: Number): FloatNumber 
		
		
		/**
		 *	Calculates the log (base 10) of a number.
		 *	@param oper The operand.
		 *	@return The base 10 log of the argument
		 */
		native static function log(oper: Number): FloatNumber 
		

		/**
		 *	Returns the greater of the number or the argument.
		 *	@param x First number to compare
		 *	@param y Second number to compare
		 *	@return A number
		 */
		native function max(x: Number, y: Number): Number


		/**
		 *	Returns the lessor of the number or the argument.
		 *	@param x First number to compare
		 *	@param y Second number to compare
		 *	@return A number
		 */
		native function min(x: Number, y: Number): Number


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	@param num The number to raise to the power
		 *	@param pow The exponent to raise @num to
		 *	@return A number
		 */
		native function power(num: Number, pow: Number): Number


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	Invokes @power.
		 *	@param limit The number to compare to
		 *	@return A number
		 */
		# ECMA
		native function pow(num: Number, pow: Number): Number


		/**
		 *	Generates a random number (a FloatNumber) inclusively between 0.0 and 1.0.
		 *	@return A random number
		 *	BUG: ECMA returns double
		 */
		native static function random(): FloatNumber 


		/**
		 *	Round this number down to the closes integral value.
		 *	@return A rounded number
		 */
		native function round(): Number


		/**
		 *	Calculates the sine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The sine of the argument 
		 */
		native static function sin(angle: Number): FloatNumber 
		

		/**
		 *	Calculates the square root of a number.
		 *	@param oper The operand.
		 *	@return The square root of the argument
		 */
		native static function sqrt(oper: Number): FloatNumber 
		

		/**
		 *	Calculates the tangent of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The tangent of the argument 
		 */
		native static function tan(angle: Number): FloatNumber 
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Math.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Name.es"
 */
/************************************************************************/

/*
 *	Name.es -- Name class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	//	TODO
	# ECMA
	native final class Name {
        use default namespace public

		public const qualifier: Namespace
		public const identifier: Namespace

		native function Name(qual: String, id: String = undefined)
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Name.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Namespace.es"
 */
/************************************************************************/

/*
 *	Namespace.es -- Namespace class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *	NOTE: this is only partially implemented.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Namespaces are used to qualify names into discrete spaces
	 */
	native final class Namespace {

        use default namespace public

		/**
		 *	Name of the namespace
		 */
		native var name: String

		/**
		 *	Unique URI defining the string used to qualify names which use the namespace
		 */
		native var uri: String

		/*
		 *	Constructor. Valid forms:
		 *		Namespace()
		 *		Namespace(uriValue: String)
		 *		Namespace(prefixValue: String, uriValue: String)
         *   native function Namespace(...args);
		 */
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Namespace.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Null.es"
 */
/************************************************************************/

/*
 *	Null.es -- Null class used for the null value.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Base type for the null value. There is only one instance of the Null type and that is the null value.
	 */
	native final class Null {

        /*
         *  Implementation artifacts
         */
		override iterator native function get(deep: Boolean = false): Iterator
		override iterator native function getValues(deep: Boolean = false): Iterator
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Null.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Number.es"
 */
/************************************************************************/

/*
 *	Number.es - Number class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

//        static const MAX_VALUE : double         = 1.7976931348623157e+308;  /* INFORMATIVE */
//        static const MIN_VALUE : double         = 5e-324;                   /* INFORMATIVE */
//        static const NaN : double               = 0.0 / 0.0;
//        static const NEGATIVE_INFINITY : double = -1.0 / 0.0;
//        static const POSITIVE_INFINITY : double = 1.0 / 0.0;
//        // 15.8.1 Value Properties of the Math Object.  These are {DD,DE,RO}.
//        static const E: double = 2.7182818284590452354;   /* Approximately */
//        static const LN10: double = 2.302585092994046;    /* Approximately */
//        static const LN2: double = 0.6931471805599453;    /* Approximately */
//        static const LOG2E: double = 1.4426950408889634;  /* Approximately */
//        static const LOG10E: double = 0.4342944819032518; /* Approximately */
//        static const PI: double = 3.1415926535897932;     /* Approximately */
//        static const SQRT1_2: double = 0.7071067811865476;/* Approximately */
//        static const SQRT2: double = 1.4142135623730951;  /* Approximately */

	/**
	 *	The Number type is used by all numeric values in Ejscript. Depending on how Ejscript is configured, the underlying
	 *	number representation may be based on either an int, long, int64 or double data type. If the underlying type is
	 *	integral (not double) then some of these routines will be mapped onto 
	 */
	native final class Number {

        use default namespace public

		/**
		 *	Number constructor.
		 *	@param value. Value to use in creating the Number object. If the value cannot be converted to a number, 
         *	    the value will ba NaN (or 0 if using integer numerics).
         *	@spec ecma-3
		 */
		native function Number(...value)


		/**
		 *	Return the maximim value this number type can assume. Alias for MaxValue.
		 *	@return An object of the appropriate number with its value set to the maximum value allowed.
		 */
		# ECMA
		static const MAX_VALUE: Number = MaxValue


		/**
		 *	Return the minimum value this number type can assume. Alias for MinValue.
		 *	@return An object of the appropriate number with its value set to the minimum value allowed.
		 */
		# ECMA
		static const MIN_VALUE: Number = MinValue


		/**
		 *	Return a unique value which is less than or equal then any value which the number can assume. 
		 *	@return A number with its value set to -Infinity. If the numeric type is integral, then return zero.
		 */
		# ECMA
		static const NEGATIVE_INFINITY: Number = NegativeInfinity


		/**
		 *	Return a unique value which is greater then any value which the number can assume. 
		 *	@return A number with its value set to Infinity. If the numeric type is integral, then return MaxValue.
		 */
		# ECMA
		static const POSITIVE_INFINITY: Number = Infinity


		/**
		 *	Return the maximim value this number type can assume.
		 *	@return A number with its value set to the maximum value allowed.
		 *	@spec ejs-11
		 */
		native static const MaxValue: Number


		/**
		 *	Return the minimum value this number type can assume.
		 *	@return A number with its value set to the minimum value allowed.
		 *	@spec ejs-11
		 */
		native static const MinValue: Number


		/**
		 *	The ceil function computes the smallest integral number that is greater or equal to the number value. 
		 *	@return A number rounded up to the next integral value.
		 *	@spec ejs-11
		 */
		native function get ceil(): Number 


		/**
		 *	Compuete the largest integral number that is  smaller than the number value.
		 *	@return A number rounded down to the closest integral value.
		 *	@spec ejs-11
		 */
		native function get floor(): Number


		/**
		 *	Returns true if the number is not Infinity or NegativeInfinity.
		 *	@return A boolean
		 *	@spec ejs-11
		 */
		native function get isFinite(): Boolean


		/**
		 *	Returns true if the number is equal to the NaN value. If the numeric type is integral, this will 
         *	    always return false.
		 *	@return A boolean
		 *	@spec ejs-11
		 */
		native function get isNaN(): Boolean


		/**
		 *	Compute the integral number that is closest to this number. Returns the closest integer value of this number 
		 *	closest to the number. ie. round up or down to the closest integer.
		 *	@return A integral number.
		 *	@spec ejs-11
		 */
		native function get round(): Number


		/**
		 *	Returns the number formatted as a string in scientific notation with one digit before the decimal point 
		 *	and the argument number of digits after it.
		 *	@param fractionDigits The number of digits in the fraction.
		 *	@return A string representing the number.
		 */


		/**
		 *	Returns the number formatted as a string with the specified number of digits after the decimal point.
		 *	@param fractionDigits The number of digits in the fraction.
		 *	@return A string
		 */
		native function toFixed(fractionDigits: Number = 0): String


		/**
		 *	Returns the number formatted as a string in either fixed or exponential notation with argument number of digits.
		 *	@param numDigits The number of digits in the result
		 *	@return A string
		 *	@spec ejs-11
		 */
		native function toPrecision(numDigits: Number = SOME_DEFAULT): String


		/**
		 *	Returns the absolute value of a number (which is equal to its magnitude).
		 *	@return the absolute value.
		 *	@spec ejs-11
		 */
		native function get abs(): Number


		/**
		 *	Convert this number to a byte sized integral number. Numbers are rounded and truncated as necessary.
		 *	@return A byte
		 *	@spec ejs-11
		 */
        # FUTURE
		function get byte(): Number {
			return integer(8)
		}


		/**
		 *	Convert this number to an integral value. Floating point numbers are converted to integral values 
         *	    using truncation.
		 *	@size Size in bits of the value
		 *	@return An integral number
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get integer(size: Number = 32): Number


		/**
		 *	Return an iterator that can be used to iterate a given number of times. This is used in for/in statements.
		 *	@param deep Ignored
		 *	@return an iterator
		 *	@example
		 *		for (i in 5) 
		 *			print(i)
		 *	@spec ejs-11
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Return an iterator that can be used to iterate a given number of times. This is used in for/each statements.
		 *	@param deep Ignored
		 *	@return an iterator
		 *	@example
		 *		for each (i in 5) 
		 *			print(i)
		 *	@spec ejs-11
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Returns the greater of the number or the argument.
		 *	@param other The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function max(other: Number): Number {
			return this > other ? this : other
		}


		/**
		 *	Returns the lessor of the number or the argument.
		 *	@param other The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function min(other: Number): Number {
			return this < other ? this : other
		}


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	@param limit The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function power(power: Number): Number {
			var result: Number = this
			for (i in power) {
				result *= result
			}
			return result
		}

		/*
			Operators: /.  for truncating division
		 */
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Number.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Object.es"
 */
/************************************************************************/

/*
 *	Object.es -- Object class. Base class for all types.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of this file.
 *

	TODO Missing:
	- function hasOwnProperty(name: EnumerableId): Boolean
	- function isPrototypeOf(value: Object): Boolean
	- function propertyIsEnumerable(name: EnumerableId): Boolean
	- function __define_Poperty__(name: EnumerableId, value, enumerable: Boolean = undefined, 
		removable: Boolean = undefined, writable: Boolean = undefined): Boolean
 */
module ejs {

	use default namespace intrinsic

	/**
	 *	The Object Class is the root class from which all objects are based. It provides a foundation set of functions 
	 *	and properties which are available to all objects. It provides for: copying objects, evaluating equality of 
	 *	objects, providing information about base classes, serialization and deserialization and iteration. 
	 */
	dynamic native class Object implements Iterable {

        use default namespace public

		/**
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function __defineProperty__(name, value, xenumerable = undefined, removable = undefined, writable = undefined)


		/**
		 *	Clone the array and all its elements.
		 *	@param deep If true, do a deep copy where all object references are also copied, and so on, recursively.
		 *	@spec ejs-11
		 */
		native function clone(deep: Boolean = true) : Array


		/**
		 *	Get an iterator for this object to be used by "for (v in obj)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ejs-11
		 */
		iterator native function get(deep: Boolean = false, namespaces: Array = null): Iterator


		/**
		 *	Get an iterator for this object to be used by "for each (v in obj)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ejs-11
		 */
		iterator native function getValues(deep: Boolean = false, namespaces: Array = null): Iterator


		/**
		 *	Check if an object has a property.
		 *	@param property Name of property to check for.
		 *	@returns true if the object contains the specified property.
		 *	Arg type should be: 
    	 *			type EnumerableId = (int|uint|string|Name)
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function hasOwnProperty(property: Object): Boolean


		/**
		 *	Is this object a prototype of the nominated argument object.
		 *	@param obj Target object to use in test.
		 *	@returns true if this is a prototype of obj.
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function isPrototypeOf(obj: Object): Boolean


		/**
		 *	The length of the object.
		 *	@return Returns the most natural size or length for the object. For types based on Object, the number of 
		 *	properties will be returned. For Arrays, the number of elements will be returned. For some types, 
		 *	the size property may be writable. For null objects the length is 0; for undefined objects the length is -1.
		 *	BUG: ECMA specifies to return 1 always
		 */
		native function get length(): Number 


	    //	TODO - this is in-flux in the spec. Better place is in the reflection API.
		/**
		 *	Test and optionally set the enumerability flag for a property.
		 *	@param property Name of property to test.
		 *	@param flag Enumerability flag. Set to true, false or undefined. Use undefined to test the current value.
		 *	@returns true if this is a prototype of obj.
		 *	@spec ecma-3
		 */
		# ECMA
		native function propertyIsEnumerable(property: String, flag: Object = undefined): Boolean


		/**
		 *	Seal a dynamic object. Once an object is sealed, further attempts to create or delete properties will fail 
		 *	and will throw an exception. 
		 *	@spec ejs-11
		 */
		# FUTURE
		native function seal() : Void


		/**
		 *	This function converts an object to a localized string representation. 
		 *	@returns a localized string representation of the object. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		native function toLocaleString(): String


		/**
		 *	Convert an object to a source code representationstring. This function returns a literal string for the object 
		 *		and all its properties. It works recursively and handles nested objects, arrays and other core types. 
		 *	@return This function returns an object literal string.
		 *	@throws TypeError If the object could not be converted to a string.
		 *	@spec ecma-3
		 */ 
		# ECMA
		function toSource(): String {
			return serialize()
		}


		/**
		 *	This function converts an object to a string representation. Types typically override this to provide 
		 *	the best string representation.
		 *	@returns a string representation of the object. For Objects "[object className]" will be returned, 
		 *	where className is set to the name of the class on which the object was based.
		 *	@spec ecma-3
		 */ 
		native function toString(locale: String = null): String


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 *	@spec ecma-3
		 */ 
		# ECMA
		function valueOf(): String {
			return this
		}
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Object.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Reflect.es"
 */
/************************************************************************/

/*
 *	Reflect.es - Reflection API and class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
	 *	Usage:
	 *
	 *	name      = Reflect(obj).name
	 *	type      = Reflect(obj).type
	 *	typeName  = Reflect(obj).typeName
     *
     *  TODO namespace
	 */

    /**
     *  Simple reflection class
     */
    native final class Reflect {
        use default namespace public

        native private var obj: Object

        /**
         *  Create a new reflection object.
         *  @param object to reflect upon
         */
        native function Reflect(o: Object)

        /**
         *  Get the name of the object
         *  @return a string describing the name of the object
         */
        native function get name(): String

        /**
         *  Get the type of the object
         *  @return the type object for object being examined.
         */
        native function get type(): Type

        /**
         *  Get the type name of the object
         *  @return a string containing the type name object for object being examined.
         */
        native function get typeName(): String

        # FUTURE
        native function get baseClass(): Type
    }

    /**
     *  Return the name of a type. This is a fixed version of the standard "typeof" operator. It returns the real
     *  Ejscript underlying type. 
     *  This is implemented as a wrapper around Reflect(o).typeName.
     *  @param o Object or value to examine. 
     *  @return A string type name. If the object to examine is a type object, then return the name of the base type.
     *      If the object is Object, then return null.
     *  @spec ejs-11
     */
    function typeOf(o): String {
        return Reflect(o).typeName()
    }


/*
 *  ES4 reflection proposal
 
	# ECMA
	intrinsic function typeOf(e: *): Type

	interface Type {
		function canConvertTo(t: Type): Boolean
		function isSubtypeOf(t: Type): Boolean
	}

	interface Field {
		function namespace() : String
		function name(): String
		function type(): Type
	}

	type FieldIterator = iterator::IteratorType.<Field>

	interface NominalType extends Type {
		function name(): String
		function namespace(): String
		function superTypes(): IteratorType.<ClassType>
		function publicMembers(): FieldIterator
		function publicStaticMembers(): FieldIterator
	}

	interface InterfaceType extends NominalType {
		function implementedBy():IteratorType.<ClassType>
	}

	type TypeIterator = iterator::IteratorType.<Type>
	type ValueIterator = iterator::IteratorType.<*>

	interface ClassType extends NominalType {
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): Object
	}

	interface UnionType extends Type {
		function members(): TypeIterator
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): *
	}

	interface FieldValue {
		function namespace() : String
		function name(): String
		function value(): *
	}

	type FieldValueIterator = iterator::IteratorType.<FieldValue>

	interface RecordType extends Type {
		function fields(): FieldIterator
		function construct(typeArgs: TypeIterator, valArgs: FieldValueIterator): Object
	}

	interface FunctionType extends Type {
		function hasBoundThis(): Boolean
		function returnType(): Type
		function argTypes(): TypeIterator
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): *
		function apply(typeArgs: TypeIterator, thisArg: Object?, valArgs: ValueIterator): *
	}
*/
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Reflect.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/RegExp.es"
 */
/************************************************************************/

/*
 *	Regex.es -- Regular expression class.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

    //  # Config.RegularExpressions - TODO 
	/**
	 *	Regular expressions per ECMA-262. Ecmascript 4 will be supported in the future. The following special 
	 *	characters are supported:
     *	<ul>
	 *		<li>\ - Reverse whether a character is treated	literally or not.</li>
	 *		<li>^ - Match to the start of input. If matching multiline, match starting after a line break.</li>
	 *		<li>$ - Match to the end of input. If matching multiline, match before after a line break.</li>
	 *		<li>~~ - Match the preceding item zero or more	times.</li>
	 *		<li>+ - Match the preceding item one or more	times.</li>
	 *		<li>? - Match the preceding item zero or one times.</li>
	 *		<li>(mem) - Match inside the parenthesis (i.e. "mem") and store the match.</li>
	 *		<li>(?:nomem) - Match "nomem" and do not store the match.</li>
	 *		<li>oper(?=need) - Match "oper" only if it is  followed by "need".</li>
	 *		<li>oper(?!not) - Match "oper" only if it is not followed by "not".</li>
	 *		<li>either|or - Match "either" or "or".</li>
	 *		<li>{int} - Match exactly int occurences of the preceding item.</li>
	 *		<li>{int,} - Match at least int occurences of the preceding item.</li>
	 *		<li>{int1,int2} - Match at least int1 occurences of the preceding item but no more then int2.</li>
	 *		<li>[pqr] - Match any one of the enclosed characters. Use a hyphen to specify a range of characters.</li>
	 *		<li>[^pqr] - Match anything except the characters in brackets.</li>
	 *		<li>[\b] - Match a backspace.</li>
	 *		<li>\b - Match a word boundary.</li>
	 *		<li>\B - Match a non-word boundary.</li>
	 *		<li>\cQ - Match a control string, e.g. Control-Q</li>
	 *		<li>\d - Match a digit.</li>
	 *		<li>\D - Match any non-digit character.</li>
	 *		<li>\f - Match a form feed.</li>
	 *		<li>\n - Match a line feed.</li>
	 *		<li>\r - Match a carriage return.</li>
	 *		<li>\s - Match a single white space.</li>
	 *		<li>\S - Match a non-white space.</li>
	 *		<li>\t - Match a tab.</li>
	 *		<li>\v - Match a vertical tab.</li>
	 *		<li>\w - Match any alphanumeric character.</li>
	 *		<li>\W - Match any non-word character.</li>
	 *		<li>\int - A reference back int matches.</li>
	 *		<li>\0 - Match a null character.</li>
	 *		<li>\xYY - Match the character code YY.</li>
	 *		<li>\xYYYY - Match the character code YYYY.</li>
     *	</ul>
	 */
	native final class RegExp {

        use default namespace public

		/**
		 *	Create a regular expression object that can be used to process strings.
		 *	@param pattern The pattern to associated with this regular expression.
		 *	@param flags "g" for global match, "i" to ignore case, "m" match over multiple lines, "y" for sticky match.
		 */
		native function RegExp(pattern: String, flags: String = null)


		/**
		 *	Get the integer index of the end of the last match plus one. This is the index to start the next match for
         *  global patterns.
		 *	@return Match end plus one or -1 if there is no last match.
		 */
		native function get lastIndex(): Number


		/**
		 *	Set the integer index of the end of the last match plus one. This is the index to start the next match for
         *  global patterns.
		 *	@return Match end plus one or -1 if there is no last match.
		 */
		native function set lastIndex(value: Number): Void


		/**
		 *	Match this regular expression against a string. By default, the matching starts at the beginning 
		 *	of the string.
		 *	@param str String to match.
		 *	@param start Optional starting index for matching.
		 *	@return Array of results, empty array if no matches.
		 *	@spec ejs-11 Adds start argument.
		 */
		native function exec(str: String, start: Number = 0): Array


		/**
		 *	Get the global flag this regular expression is using. If the global flag is set, the regular expression 
		 *	will search through the entire input string looking for matches.
		 *	@return The global flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get global(): Boolean


		/**
		 *	Get the case flag this regular expression is using. If the ignore case flag is set, the regular expression 
		 *	is case insensitive.
		 *	@return The case flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get ignoreCase(): Boolean


		/**
		 *	Get the multiline flag this regular expression is using. If the multiline flag is set, the regular 
		 *	expression will search through carriage return and new line characters in the input.
		 *	@return The multiline flag, true if set, false otherwise.
		 */
		native function get multiline(): Boolean


		/**
		 *	Get the regular expression source pattern is using to match with.
		 *	@return The pattern string
		 */
		native function get source(): String


		/**
		 *	Get the substring that was last matched.
		 *	@return The matched string or null if there were no matches.
		 *	@spec ejs-11
		 */
		native function get matched(): String


		/**
		 *	Replace all the matches. This call replaces all matching substrings with the corresponding array element.
		 *	If the array element is not a string, it is converted to a string before replacement.
		 *	@param str String to match and replace.
		 *	@return A string with zero, one or more substitutions in it.
		 *	@spec ejs-11
		 */
		function replace(str: String, replacement: Object): String {
            return str(this, replacement)
        }


		/**
		 *	Split the target string into substrings around the matching sections.
		 *	@param String to split.
		 *	@return Array containing the matching substrings
		 *	@spec ejs-11
		 */
		function split(target: String): Array {
            return target.split(this)
        }


		/**
		 *	Get the integer index of the start of the last match.
		 *	@return Match start.
		 *	@spec ejs-11
		 */
		native function get start(): Number


		/**
		 *	Get the sticky flag this regular expression is using. If the sticky flag is set, the regular expression 
		 *	contained the character flag "y".
		 *	@return The sticky flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get sticky(): Boolean


		/**
		 *	Test whether this regular expression will match against a string.
		 *	@param str String to search.
		 *	@return True if there is a match, false otherwise.
		 *	@spec ejs-11
		 */
		native function test(str: String): Boolean


		/**
		 *	Convert the regular expression to a string
		 *	@param locale Locale 
		 *	@returns a string representation of the regular expression.
		 */
		override native function toString(locale: String = null): String
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/RegExp.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Stream.es"
 */
/************************************************************************/

/*
 *	Stream.es -- Stream class. Base interface implemented by Streams.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

    use default namespace intrinsic

	/**
	 *	Stream objects represent bi-directional streams of data that pass data elements between an endpoint known as a source 
	 *	or sink and a consumer / producer. In between, intermediate streams may be used as filters. Example endpoints are
	 *	the File, Socket, String and Http classes. The TextStream is an example of a filter stream. The data elements passed 
     *	by streams may be any series of objects including: bytes, lines of text, integers or objects. Streams may buffer the 
     *	incoming data or not. Streams may offer sync and/or async modes of operation.
	 *	@spec ejs-11
	 */
	interface Stream {

        use default namespace public

		/**
		 *	Close the input stream and free up all associated resources.
		 *	@param graceful if true, then close the socket gracefully after writing all pending data.
		 */
		function close(graceful: Boolean = false): Void


		/**
		 *	Flush the stream and all stacked streams and underlying data source/sinks.
		 */
		function flush(): Void 


		/**
		 *	Read a block of data from the stream. Read the required number of bytes from the stream into the supplied byte 
		 *	array at the given offset. 
		 *	@param count Number of elements to read. 
		 *	@returns a count of the bytes actually read.
		 *	@throws IOError if an I/O error occurs.
		 */
		function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number 


		/**
		 *	Write data to the stream. If in sync mode, the write call blocks until the underlying stream or endpoint absorbes 
		 *	all the data. If in async-mode, the call accepts whatever data can be accepted immediately and returns a count of 
		 *	the elements that have been written.
		 *	@param data Data to write. 
		 *	@returns The total number of elements that were written.
		 *	@throws IOError if there is an I/O error.
		 */
		function write(... data): Number
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Stream.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/String.es"
 */
/************************************************************************/

/*
 *	String.es -- String class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Each String object represents a single immutable linear sequence of characters. Strings have operators 
	 *	for: comparison, concatenation, copying, searching, conversion, matching, replacement, and, subsetting.
	 */
	native final class String {

        use default namespace public

		/**
		 *	String constructor. This can take two forms:
		 *	<ul>
		 *		<li>String()</li>
		 *		<li>String(str: String)</li>
		 *	</ul>
		 *	@param The args can be either empty or a string. If a non-string arg is supplied, the VM will automatically
         *	    cast to a string.
		 */
		native function String(...str)


		/**
		 *	Do a case sensitive comparison between this string and another.
		 *	@param The string to compare against
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		native function caseCompare(compare: String): Number


		/**
		 *	Return the character at a given position in the string
		 *	@returns a new string containing the selected character.
		 *	@throws RangeException
		 *	@spec ecma-3
		 */
		native function charAt(index: Number): String


		/**
		 *	Get a character code. 
		 *	@param The index of the character to retrieve
		 *	@return Return the character code at the specified index. If the index is -1, get the last character.
		 *	@throws OutOfBoundsError If the index is less then -1 or greater then or equal to the size of string.
		 *	@spec ecma-3
		 */
		native function charCodeAt(index: Number = 0): Number


		/**
		 *	Concatenate strings and returns a new string. 
		 *	@param args Strings to append to this string
		 *	@return Return a new string.
		 *	@spec ecma-3
		 */
		native function concat(...args): String


        //  TODO - change to (String | RegExp)
		/**
		 *	Check if a string contains a pattern.
		 *	@param pattern The pattern can be either a string or regular expression.
		 *	@return Returns true if the pattern is found.
		 *	@spec ejs-11
		 */
		native function contains(pattern: Object): Boolean


		/**
		 *	Determine if this string ends with a given string
		 *	@param test The string to test with
		 *	@return True if the string matches.
		 *	@spec ejs-11
		 */
		native function endsWith(test: String): Boolean


		/**
		 *	Format arguments as a string. Use the string as a format specifier.
		 *	@param args Array containing the data to format. 
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@example
		 *		"%5.3f".format(num)
         *  \n\n
		 *		"%s Arg1 %d, arg2 %d".format("Hello World", 1, 2)
		 *	@spec ejs-11
		 */
		native function format(...args): String


		/**
		 *	Create a string from the character code arguments
		 *	@param codes Character codes from which to create the string
		 *	@returns a new string
		 *	@spec ecma-3
		 */
		native static function fromCharCode(...codes): String
		

		/**
		 *	Get an iterator for this array to be used by "for (v in string)"
		 *	@param deep Follow the prototype chain. Not used.
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in string)"
		 *	@param deep Follow the prototype chain. Not used.
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Search for an item using strict equality "===". This call searches from the start of the string for 
		 *	the specified element. 
		 *	@param pattern The string to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function indexOf(pattern: String, startIndex: Number = 0): Number


		/**
		 *	If there is at least one character in the string and all characters are digits return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isDigit(): Boolean


        //  TODO - need isAlphaNum
		/**
		 *	If there is at least one character in the string and all characters are alphabetic return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isAlpha(): Boolean


		/**
		 *	If there is at least one character in the string that can be upper or lower case and all characters 
		 *	are lower case return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isLower(): Boolean


		/**
		 *	If there is at least one character in the string and all characters are white space return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isSpace(): Boolean


		/**
		 *	If there is at least one character in the string that can be upper or lower case and all characters are upper 
		 *	case return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isUpper(): Boolean


		/**
		 *	Search right to left for a substring starting at a given index.
		 *	@param pattern The string to search for
		 *	@param location The integer starting to start the search or a range to search in.
		 *	@throws OutOfBoundsError If the index is less then -1 or greater then or equal to the size of string.
		 *	@return Return the starting index of the last match found.
		 *	@spec ecma-3
		 */
		native function lastIndexOf(pattern: String, location: Number = -1): Number


		/**
		 *	Get the length of a string. 
		 *	@return Return the length of the string in characters.
		 *	@spec ecma-3
		 */
		override native function get length(): Number


		/**
		 *	Compare another string with the this string object
		 *	@returns zero if the strings are equal, -1 if this string is lexically before @str and 1 if after.
		 *	@spec ecma-3
		 */
		# ECMA
		function localeCompare(str: String): Number {
			if (this < string) {
				return -1
			} else if (string == this) {
				return 0
			} else {
				return 1
			}
		}


        //  # Config.RegularExpressions - TODO
        //  TODO - should this allow a string?
		/**
		 *	Match the a regular expression pattern against a string.
		 *	@param pattern The regular expression to search for
		 *	@return Returns an array of matching substrings.
		 *	@spec ecma-3, ejs-11 allows pattern to be a string
		 */
		native function match(pattern: RegExp): Array


		/**
		 *	Parse the current string object as a JSON string object. The @filter is an optional filter with the 
		 *	following signature:
		 *		function filter(key: String, value: String): Boolean
		 *	@param filter Function to call for each element of objects and arrays.
		 *	@returns an object representing the JSON string.
		 *	@spec ecma-3
		 */
		# ECMA
		native function parseJSON(filter: Function): Object


		/**
		 *	Copy the string into a new string and lower case the first letter if there is one. If the first non-white 
		 *	character is not a character or if it is already lower there is no change.
		 *	@return A new String
		 *	@spec ejs-11
		 */
		native function toCamel(): String


		/**
		 *	Copy the string into a new string and capitalize the first letter if there is one. If the first non-white 
		 *	character is not a character or if it is already capitalized there is no change.
		 *	@return A new String
		 *	@spec ejs-11
		 */
		native function toPascal(): String


		/**
		 *	Create a new string with all nonprintable characters replaced with unicode hexadecimal equivalents (e.g. \uNNNN).
		 *	@return The new string
		 *	@spec ejs-11
		 */
		native function printable(): String


		/**
		 *	Wrap a string in double quotes.
		 *	@return The new string
		 *	@spec ecma-3
		 */
		native function quote(): String


		/**
		 *	Remove characters from a string. Remove the elements from @start to @end inclusive. 
		 *	@param start Numeric index of the first element to remove. Negative indicies measure from the end of the string.
         *	-1 is the last character element.
		 *	@param end Numeric index of one past the last element to remove
		 *	@return A new string with the characters removed
		 *	@spec ejs-11
		 */
		native function remove(start: Number, end: Number = -1): String


		/**
		 *	Search and replace. Search for the given pattern which can be either a string or a regular expression 
		 *	and replace it with the replace text.
		 *	@param pattern The regular expression pattern to search for
		 *	@param replacement The string to replace the match with or a function to generate the replacement text
		 *	@return Returns a new string.
		 *	@spec ejs-11
		 */
		native function replace(pattern: Object, replacement: String): String


		/**
		 *	Reverse a string. 
		 *	@return Returns a new string with the order of all characters reversed.
		 *	@spec ejs-11
		 */
		native function reverse(): String

	
		/**
		 *	Search for a pattern.
		 *	@param pattern Regular expression pattern to search for in the string.
		 *	@return Return the starting index of the pattern in the string.
		 *	@spec ecma-3
		 */
		native function search(pattern: Object): Number


		/**
		 *	Extract a substring.
		 *	@param start The position of the first character to slice.
		 *	@param end The position one after the last character. Negative indicies are measured from the end of the string.
		 *	@throws OutOfBoundsError If the range boundaries exceed the string limits.
		 *	@spec ecma-3
		 */	
		native function slice(start: Number, end: Number = -1, step: Number = 1): String


		/**
		 *	Split a string into an array of substrings. Tokenizes a string using a specified delimiter.
		 *	@param delimiter String or regular expression object separating the tokens.
		 *	@param limit At most limit strings are extracted. If limit is set to -1, then unlimited strings are extracted.
		 *	@return Returns an array of strings.
		 *	@spec ecma-3
		 */
		native function split(delimiter: Object, limit: Number = -1): Array


		/**
		 *	Tests if this string starts with the string specified in the argument.
		 *	@param test String to compare against
		 *	@return True if it does, false if it doesn't
		 *	@spec ejs-11
		 */
		native function startsWith(test: String): Boolean


		/**
		 *	Extract a substring. Similar to substring, but utilizes a length.
		 *	@param startIndex Integer location to start copying
		 *	@param length Number of characters to copy
		 *	@return Returns a new string
		 *	@throws OutOfBoundsError If the starting index and/or the length exceed the string's limits.
		 *	@spec ecma-3
		 */
		# ECMA
		native function substr(startIndex: Number, length: Number = -1): String


		/**
		 *	Extract a substring. Similar to slice but only allows positive indicies.
		 *	@param startIndex Integer location to start copying
		 *	@param end Postitive index of one past the last character to extract.
		 *	@return Returns a new string
		 *	@throws OutOfBoundsError If the starting index and/or the length exceed the string's limits.
		 *	@spec ecma-3
		 */
		native function substring(startIndex: Number, end: Number = -1): String


		/**
		 *	Replication. Replicate the string N times.
		 *	@param str The number of times to copy the string
		 *	@return A new String with the copies concatenated together
		 *	@spec ejs-11
		 */
		function times(times: Number): String {
			var s: String = ""

			for (i in times) {
				s += this
			}
			return s
		}


		/**
		 *	Serialize the string as a JSON string.
		 *	@returns a string containing the string serialized as a JSON string.
		 *	@spec ecma-3
		 */ 
		# ECMA
		native function toJSONString(pretty: Boolean = false): String


		//	TODO. Should this be the reverse?   for (s in "%s %s %s".tokenize(value))
        //  that would be more consistent with format()
		/**
		 *	Scan the input and tokenize according to a string format specifier.
		 *	@param format Tokenizing format specifier
		 *	@returns array containing the tokenized elements
		 *	@example
		 *		for (s in string.tokenize("%s %s %s")) {
		 *			print(s)
		 *		}
		 *	@spec ejs-11
		 */
		native function tokenize(format: String): Array


		/**
		 *	Convert the string to lower case.
		 *	@return Returns a new lower case version of the string.
		 *	@spec ejs-11
		 */
		native function toLower(locale: String = null): String


		/**
		 *	This function converts the string to a localized lower case string representation. 
		 *	@returns a lower case localized string representation of the string. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		function toLocaleLower(): String {
			//	TODO should be getting from App.Locale not from date
			return toLowerCase(Date.LOCAL)
		}


		/**
		 *	This function converts the string to a localized string representation. 
		 *	@returns a localized string representation of the string. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		override function toLocaleString(): String {
			return toString()
		}


		/**
		 *	This function converts an object to a string representation. Types typically override this to provide 
		 *	the best string representation.
		 *	@returns a string representation of the object. For Objects "[object className]" will be returned, 
		 *	where className is set to the name of the class on which the object was based.
		 *	@spec ecma-3
		 */ 
		override native function toString(locale: String = null): String


		/**
		 *	Convert the string to upper case.
		 *	@return Returns a new upper case version of the string.
		 *	@spec ejs-11
		 */
		native function toUpper(locale: String = null): String


		/**
		 *	Convert the string to localized upper case string
		 *	@return Returns a new localized upper case version of the string.
		 *	@spec ecma-3
		 */
		# ECMA
		function toLocaleUpperCase(locale: String = null): String {
			//	TODO should be getting from App.Locale not from date
			return toUpper(Date.LOCAL)
		}


//  TODO - great if this could take a regexp
		/**
		 *	Returns a trimmed copy of the string. Normally used to trim white space, but can be used to trim any 
		 *	substring from the start or end of the string.
		 *	@param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
		 *	@return Returns a (possibly) modified copy of the string
		 *	@spec ecma-3
		 */
		native function trim(str: String = null): String


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 *	@spec ecma-3
		 */ 
		# ECMA
		override function valueOf(): String {
			return this
		}


		/*
		 *	Overloaded operators
		 */

		/**
		 *	String subtraction. Remove the first occurance of str.
		 *	@param str The string to remove from this string
		 *	@return Return a new string.
		 *	@spec ejs-11
		 */
		function - (str: String): String {
			var i: Number = indexOf(str)
			return remove(i, i + str.length)
		}

		
		//	TODO - delegate to localeCompare
		/**
		 *	Compare strings. 
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function < (str: String): Number {
			return localeCompare(str) < 0
		}


		/**
		 *	Compare strings.
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function > (str: String): Number {
			return localeCompare(str) > 0
		}


		/**
		 *	Compare strings.
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function == (str: String): Number {
			return localeCompare(str) == 0
		}


		/**
		 *	Format arguments as a string. Use the string as a format specifier.
		 *	@param arg The arguments to format. Pass an array if multiple arguments are required.
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@example
		 *		"%5.3f" % num
         *  <br/>
		 *		"Arg1 %d, arg2 %d" % [1, 2]
		 *	@spec ejs-11
		 */
		function % (obj: Object): String {
			return format(obj)
		}
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/String.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Type.es"
 */
/************************************************************************/

/*
 *	Type.es -- Type class. Base class for all type objects.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

		Must not define properties and methods of this type. They cannot be inherited by types.
	 */

	/**
	 *    Base class for all type objects.
	 */
	native final class Type {

        use default namespace public

		/**
		 *	Get the prototype object for the type. The prototype object provides the template of instance properties 
		 *	shared by all Objects.
		 */
		# TODO || ECMA
		native function get prototype(): Type


		/**
		 *	Get the base class for this type.
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get baseClass(): Type


		/**
		 *	Mix in a type into a dynamic type or object. This routine blends the functions and properties from 
		 *	a supplied mix-type into the specified target type. Mix-types that are blended in this manner are 
		 *	known as "mixins". Mixins are used to add horizontal functionality to types at run-time. The target 
		 *	type must be declared as dynamic.
		 *	@param type module The module to mix in.
		 *	@return Returns "this".
		 *	@throws StateError if the mixin fails due to the target type not being declared as dynamic.
		 *	@throws TypeError if the mixin fails due to a property clash between the mixin and target types.
		 *	@spec ejs-11
		 */ 
		# FUTURE
		native function mixin(mixType: Type): Object


		/**
		 *	Get the type (class)  name
		 *	@returns the class name
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get name(): String


		/**
		 *	Get the class name.
		 *	Same as name()
		 *	@returns the class name
		 */
		# ECMA || FUTURE
		function getClass(): String {
			return name()
		}


		/**
		 *	Seal a type. Once an type is sealed, further attempts to create or delete properties will fail, or calls 
		 *	to mixin will throw an exception. 
		 *	@spec ejs-11
		 */
		# FUTURE
		override native function seal() : void
	}


	/*
	 *	TODO - should rename class Type into class Class
	 */
	# ECMA || FUTURE
	var Class = Type
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Type.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/core/Void.es"
 */
/************************************************************************/

/*
 *	Void.es -- Void class used for undefined value.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
     *  The Void type is the base class for the undefined value. One instance of this type is created for the 
     *  system's undefined value.
	 */
	native final class Void {
        /*
         *  Implementation artifacts
         */
		override iterator native function get(deep: Boolean = false): Iterator
		override iterator native function getValues(deep: Boolean = false): Iterator
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/core/Void.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/db/Database.es"
 */
/************************************************************************/

/**
 *	Database.es -- Database class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.db {


    /**
     *  SQLite database support
     */
	class Database {

        use default namespace public

		private static var _defaultDb: Database
		private static var _traceSql: Boolean

		private var _name: String
		private var _connection: String

		/*
         *  TODO FUTURE DOC AND USAGE
         *
		 *	Initialize a database connection using the supplied database connection string
		 *	@param connectionString Connection string stipulating how to connect to the database. The format is one of the 
		 *	following forms:
		 *		<ul>
		 *			<li>adapter://host/database/username/password</li>
		 *			<li>filename</li>
		 *		</ul>
		 *		Where adapter specifies the kind of database. Sqlite is currently the only supported adapter.
		 *		For sqlite connection strings, the abbreviated form is permitted where a filename is supplied and the 
		 *		connection string is assumed to be: <pre>sqlite://localhost/filename</pre>
		 */
		/**
		 *	Initialize a SQLite database connection using the supplied database connection string.
		 *	@param connectionString Connection string stipulating how to connect to the database. The format is one of the 
		 *	following forms:
		 *		<ul>
		 *			<li>filename</li>
		 *		</ul>
         *  In the future, connection strings will support other databases using a format a format like:
		 *		<ul>
		 *			<li>adapter://host/database/username/password</li>
		 *			<li>filename</li>
		 *		</ul>
		 */
		native function Database(connectionString: String)


		/**
		 *	Reconnect to the database using a new connection string
		 *	@param connectionString See Database() for information about connection string formats.
		 */
		native function connect(connectionString: String): Void


		/**
		 *	Close the database connection. Database connections should be closed when no longer needed rather than waiting
		 *	for the garbage collector to automatically close the connection when disposing the database instance.
		 */
		native function close(): Void


		/**
		 *	Execute a SQL command on the database. This is a low level SQL command interface that bypasses logging.
         *	    Use @query instead.
		 *	@param sql SQL command string
		 *	@returns An array of row results where each row is represented by an Object hash containing the column names and
		 *		values
		 */
		native function sql(cmd: String): Array


		/**
		 *	Execute a SQL command on the database.
		 *	@param sql SQL command string
		 *	@returns An array of row results where each row is represented by an Object hash containing the column names and
		 *		values
         */
        function query(cmd: String): Array {
            log(cmd)
            return sql(cmd)
        }


		/**
		 *	Get the database connection string
		 */
		function get connection(): String {
			return _connection
		}


        /**
         *  Get the name of the database.
         *  @returns the database name defined via the connection string
         */
		function get name(): String {
			return _name
		}


        /**
         *  Return list of tables in a database
         *  @returns an array containing list of table names present in the currently opened database.
         */
		function getTables(): Array {
			let cmd: String = "SELECT name from sqlite_master WHERE type = 'table' order by NAME;"
			let grid: Array = query(cmd)
			let result: Array = new Array
			for each (let row: Object in grid) {
				let name: String = row["name"]
				if (!name.contains("sqlite_") && !name.contains("_Ejs")) {
					result.append(row["name"])
				}
			}
			return result
		}


        function getColumns(table: String): Array {
            grid =  query('PRAGMA table_info("' + table + '");')
            let names = []
            for each (let row in grid) {
                let name: String = row["name"]
                names.append(name)
            }
            return names
        }


        /**
         *  Get the default database for the application.
         *  @returns the default database defined via the $defaultDatabase setter method
         */
        static function get defaultDatabase(): Database {
            return _defaultDb
        }


        /**
         *  Set the default database for the application.
         *  @param the default database to define
         */
        static function set defaultDatabase(db: Database): Void {
            _defaultDb = db
        }


        /*
         *  Map independent types to SQLite types
         */
        static const DatatypeToSqlite: Object = {
            "binary":       "blob",
            "boolean":      "tinyint",
            "date":         "date",
            "datetime":     "datetime",
            "decimal":      "decimal",
            "float":        "float",
            "integer":      "int",
            "number":       "decimal",
            "string":       "varchar",
            "text":         "text",
            "time":         "time",
            "timestamp":    "datetime",
        }


        /*
         *  Map independent types to SQLite types
         */
        static const SqliteToDatatype: Object = {
            "blob":         "binary",
            "tinyint":      "boolean",
            "date":         "date",
            "datetime":     "datetime",
            "decimal":      "decimal",
            "float":        "float",
            "int":          "integer",
            "varchar":      "string",
            "text":         "text",
            "time":         "time",
        }


        /*
         *  Map SQLite types to Ejscript native types
         */
        static const SqliteToEjs: Object = {
            "blob":         String,
            "date":         Date,
            "datetime":     Date,
            "decimal":      Number,
            "integer":      Number,
            "float":        Number,
            "time":         Date,
            "tinyint":      Boolean,
            "text":         String,
            "varchar":      String,
        }

        /*
         *  Map Ejscript native types back to SQLite types
         *  INCOMPLETE and INCORRECT
        static const EejsToDatatype: Object = {
            "string":       "varchar",
            "number":       "decimal",
            "date":         "datetime",
            "bytearray":    "Blob",
            "boolean":      "tinyint",
        }
         */

        function createDatabase(name: String, options: Object = null): Void {
            //  Nothing to do for sqlite
        }


        function destroyDatabase(name: String): Void {
            //  TODO
        }


        //  TODO - should these types be type objects not strings?
        function createTable(table: String, columns: Array = null): Void {
            let cmd: String

            query("DROP TABLE IF EXISTS " + table + ";")
            query("CREATE TABLE " + table + "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL);")

            if (columns) {
                //  TODO - BUG should not need when null/undefined have iterators
                for each (let colspec: String in columns) {
                    //  TODO - destructuring assignment would be good here
                    let spec: Array = colspec.split(":")
                    if (spec.length != 2) {
                        throw "Bad column spec: " + spec
                    }
                    let column: String = spec[0]
                    let datatype: String = spec[1]
                    addColumn(table, column.trim(), datatype.trim())
                }
            }
        }


        function renameTable(oldTable: String, newTable: String): Void {
            query("ALTER TABLE " + oldTable + " RENAME TO " + newTable + ";")
        }


        function destroyTable(table: String): Void {
            query("DROP TABLE IF EXISTS " + table + ";")
        }


        function addIndex(table: String, column: String, indexName: String): Void {
            query("CREATE INDEX " + indexName + " ON " + table + " (" + column + ");")
        }


        function removeIndex(table: String, indexName: String): Void {
            query("DROP INDEX " + indexName + ";")
        }


        //  TODO - should these types be type objects not strings?
        function addColumn(table: String, column: String, datatype: String, options: Object = null): Void {
            datatype = DatatypeToSqlite[datatype.toLower()]
            if (datatype == undefined) {
                throw "Bad Ejscript column type: " + datatype
            }
            query("ALTER TABLE " + table + " ADD " + column + " " + datatype)
        }


        //  TODO - should these types be type objects not strings?
        function changeColumn(table: String, column: String, datatype: String, options: Object = null): Void {
            datatype = datatype.toLower()
            if (DatatypeToSqlite[datatype] == undefined) {
                throw "Bad column type: " + datatype
            }
            /*
                query("ALTER TABLE " + table + " CHANGE " + column + " " + datatype)
            */
            throw "SQLite does not support column changes"
        }


        function renameColumn(table: String, oldColumn: String, newColumn: String): Void {
            query("ALTER TABLE " + table + " RENAME " + oldColumn + " TO " + newColumn + ";")
        }


        function removeColumns(table: String, columns: Array): Void {
            /* NORMAL SQL
             * for each (column in columns)
             *   query("ALTER TABLE " + table + " DROP " + column + ";")
             */

            /*
             *  This is a dumb SQLite work around because it doesn't have drop column
             */
            backup = "_backup_" + table
            keep = getColumns(table)
            for each (column in columns) {
                if ((index = keep.indexOf(column)) < 0) {
                    throw "Column \"" + column + "\" does not exist in " + table
                } 
                keep.remove(index)
            }

            //  TODO - good to have a utility routine for this
            schema = 'PRAGMA table_info("' + table + '");'
            grid = query(schema)
            types = {}
            for each (let row in grid) {
                let name: String = row["name"]
                types[name] = row["type"]
            }

            columnSpec = []
            for each (k in keep) {
                if (k == "id") {
                    columnSpec.append(k + " INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL")
                } else {
                    columnSpec.append(k + " " + types[k])
                }
            }

            cmd = "BEGIN TRANSACTION;
                CREATE TEMPORARY TABLE " + backup + "(" + columnSpec + ");
                INSERT INTO " + backup + " SELECT " + keep + " FROM " + table + ";
                DROP TABLE " + table + ";
                CREATE TABLE " + table + "(" + columnSpec + ");
                INSERT INTO " + table + " SELECT " + keep + " FROM " + backup + ";
                DROP TABLE " + backup + ";
                COMMIT;"
            query(cmd)
        }


        private static function log(cmd: String): Void {
            if (_traceSql) {
                print("SQL: " + cmd)
            }
        }


        //  TODO - should this be static or instance
        /**
         *  Trace SQL statements. Control whether trace is enabled for the actual SQL statements issued against the database.
         *  @param on If true, display each SQL statement to the log
         */
        static function trace(on: Boolean): void {
            _traceSql = on
        }


/*
 *		FUTURE
		function map(): Void {}
		function map(table: String, ...): Void {}
		function map(tables: Array): Void {}

		function transaction(code: Function): Void {
			startTransaction()
			try {
				code()
			}
			catch (e: Error) {
				rollback();
			}
			finally {
				endTransaction()
			}
		}

		function startTransaction(): Void {}
		function rollback(): Void {}
		function commit(): Void {}
*/
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/db/Database.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/db/Record.es"
 */
/************************************************************************/

/**
 *  Record.es -- Record class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  Issues:
 *      Types. How do we convert and where
 *      Pluralize. Get class name, table name ... and convert from one to the other
 */

module ejs.db {

    /**
     *  Database record class. A record instance corresponds to a row in the database. This class provides a low level 
     *  Object Relational Mapping (ORM) between the database and Ejscript objects. This class provides methods to create,
     *  read, update and delete rows in the database. When read or initialized object properties are dynamically created 
     *  in the Record instance for each column in the database table. Users should subclass the Record class for each 
     *  database table to manage.
     */
    dynamic class Record {

        use default namespace public

        /*
         *  Trace sql statements
         */
        private static var  _belongsTo: Object
        private static var  _className: String
        private static var  _columnNames: Object
        private static var  _columnTypes: Object
        private static var  _sqlColumnTypes: Object
        private static var  _db: Database = undefined
        private static var  _foreignId: String
        private static var  _keyName: String
        private static var  _tableName: String
        private static var  _traceSql: Boolean = false
        private static var  _validations: Array

        private static var  _beforeFilters: Array
        private static var  _afterFilters: Array
        private static var  _wrapFilters: Array

        private var         _keyValue: Object
        private var         _errors: Object

        /************************************ Instance Methods ********************************/
        /**
         *  Construct a new record instance. This is really a constructor function, because the Record class is implemented 
         *  by user models, no constructor will be invoked when a new user model is instantiated. The record may be 
         *  initialized by optionally supplying field data. However, the record will not be written to the database 
         *  until $save is called. To read data from the database into the record, use one of the $find methods.
         *  @param fields An optional object set of field names and values may be supplied to initialize the recrod.
         */
        function constructor(fields: Object = null): Void {
            if (_columnNames == null) {
                getSchema()
            }

            //  TODO - need to validate the fields here and only create valid coluns - also need to get the types

            if (fields) {
                for (field in fields) {
                    if (_columnNames[field] == undefined) {
                        throw new Error("Column " + field + " is not a column in the table \"" + _tableName + "\"")
                    } else {
                        this[field] = fields[field]
                    }
                }
            }
        }

        static function beforeFilter(fn, options: Object = null): Void {
            if (_beforeFilters == undefined) {
                _beforeFilters = []
            }
            _beforeFilters.append([fn, options])
        }

        static function afterFilter(fn, options: Object = null): Void {
            if (_afterFilters == undefined) {
                _afterFilters = []
            }
            _afterFilters.append([fn, options])
        }

        static function wrapFilter(fn, options: Object = null): Void {
            if (_wrapFilters == undefined) {
                _wrapFilters = []
            }
            _wrapFilters.append([fn, options])
        }

        private function runFilters(filters): Void {
            if (!filters) {
                return
            }
            for each (filter in filters) {
                let fn = filter[0]
                let options = filter[1]
                if (options) {
                    only = options.only
/*
                    if (only) {
                        if (only is String && actionName != only) {
                            continue
                        }
                        if (only is Array && !only.contains(actionName)) {
                            continue
                        }
                    } 
                    except = options.except
                    if (except) {
                        if (except is String && actionName == except) {
                            continue
                        }
                        if (except is Array && except.contains(actionName)) {
                            continue
                        }
                    }
*/
                }
                fn.call(this)
            }
        }

        /**
         *  Save the record to the database.
         *  @returns True if the record is validated and successfully written to the database
         *  @throws IOError Throws exception on sql errors
         */
        //  TODO - need to know the ID that was saved? - what happens on db errors?
        function save(): Boolean {
            var sql: String

            if (!validateModel()) {
                return false
            }

            runFilters(_beforeFilters)
            
            if (_keyValue == null) {
                sql = "INSERT INTO " + _tableName + " ("
                for (let field: String in this) {
                    if (_columnNames[field]) {
                        sql += field + ", "
                    }
                }
                sql = sql.trim(', ')
                sql += ") VALUES("
                for (let field: String in this) {
                    sql += "'" + this[field] + "', "
                }
                sql = sql.trim(', ')
                sql += ")"

            } else {
                sql = "UPDATE " + _tableName + " SET "
                for (let field: String in this) {
                    if (_columnNames[field]) {
                        sql += field + " = '" + this[field] + "', "
                    }
                }
                sql = sql.trim(', ')
                sql += " WHERE " + _keyName + " = " +  _keyValue
            }
            sql += "; SELECT last_insert_rowid();"

            log("save", sql)
            //  TODO - what should be done on errors?
            let result: Array = getDb().query(sql)
            _keyValue = this["id"] = result[0]["last_insert_rowid()"] cast Number

            runFilters(_afterFilters)

            return true
        }


        /**
         *  Update a record based on the supplied fields and values.
         *  @param fields Hash of field/value pairs to use for the record update.
         *  @returns True if the record is successfully updated. Returns false if validation fails and the record is not 
         *      saved.
         *  @throws IOError on database SQL errors
         */
        function saveUpdate(fields: Object): Boolean {
            for (field in fields) {
                if (this[field] != undefined) {
                    this[field] = fields[field]
                }
            }
            return save()
        }


        /************************************ Static Methods ********************************/
        //  TODO - options are ignored
        /**
         *  Define a belonging reference to another model class. When a model belongs to another, it has a foreign key
         *  reference to another class.
         *  @param owner Referenced model class that logically owns this model.
         *  @param options Optional options hash
         *  @option className 
         *  @option foreignKey Key name for the foreign key
         *  @option conditions SQL conditions for the relationship to be satisfied
         */
        static function belongsTo(owner, options: Object = null): Void {
            if (_belongsTo == undefined) {
                _belongsTo = [owner]
            } else {
                _belongsTo.append(owner)
            }
        }


        /*
         *  Create a new record instance and apply the row data
         *  Process a sql result and add properties for each field in the row
         */
        private static function createRecord(rowData: Object): Record {
            let rec: Record = new global[_className]
            rec.constructor(rec)
            for (let field: String in rowData) {
                rec[field] = rowData[field]
            }
            rec.coerceTypes()
            rec._keyValue = rowData[_keyName]
            return rec
        }


        /**
         *  Find a record. Find and return a record identified by its primary key if supplied or by the specified options. 
         *  If more than one record matches, return the first matching record.
         *  @param key Key Optional key value. Set to null if selecting via the options 
         *  @param options Optional search option values
         *  @returns a model record or null if the record cannot be found.
         *  @throws IOError on internal SQL errors
         *
         *  @option columns List of columns to retrieve
         *  @option conditions { field: value, ...}   or [ "SQL condition", "id == 23", ...]
         *  @option from Low level from clause (TODO not fully implemented)
         *  @option keys [set of matching key values]
         *  @option order ORDER BY clause
         *  @option group GROUP BY clause
         *  @option limit LIMIT count
         *  @option offset OFFSET count
         *  @option include { table1: { table2: { table3: {}}}}
         *  @option joins Low level join statement "LEFT JOIN vists on stockId = visits.id"
         *  @option joinBelongs Automatically join all belongsTo models. Defaults to true.
         *
         *  FUTURE
         *  @option readonly
         *  @option lock
         */
        static function find(key: Object, options: Object = null): Object {
            let grid: Array = innerFind(key, options)
            if (grid.length >= 1) {
                return createRecord(grid[0])
            } 
            return null
        }


        /**
         *  Find all the matching records
         *  @param options Optional set of options. See $find for list of possible options.
         *  @returns An array of model records. The array may be empty if no matching records are found
         *  @throws IOError on internal SQL errors
         */
        static function findAll(options: Object = null): Array {
            let grid: Array = innerFind(null, options)
            for (let i = 0; i < grid.length; i++) {
                grid[i] = createRecord(grid[i])
            }
            return grid
        }


        /**
         *  Find the first record matching a condition. Select a record using a given SQL where clause.
         *  @param where SQL WHERE clause to use when selecting rows.
         *  @returns a model record or null if the record cannot be found.
         *  @throws IOError on internal SQL errors
         *  @example
         *      rec = findOneWhere("cost < 200")
         */
        static function findOneWhere(where: String): Object {
            let grid: Array = innerFind(null, { conditions: [where]})
            if (grid.length >= 1) {
                return createRecord(grid[0])
            } 
            return null
        }


        /**
         *  Find records matching a condition. Select a set of records using a given SQL where clause
         *  @param whereClause SQL WHERE clause to use when selecting rows.
         *  @returns An array of objects. Each object represents a matching row with fields for each column.
         *  @example
         *      list = findWhere("cost < 200")
         */
        static function findWhere(where: String, count: Number = null): Array {
            let grid: Array = innerFind(null, { conditions: [where]})
            for (let i = 0; i < grid.length; i++) {
                grid[i] = createRecord(grid[i])
            }
            return grid
        }


        //  TODO
        private static function includeJoin(joins: Object): String {
            var cmd: String = ""
            for each (join in joins) {
                cmd ="LEFT OUTER JOIN " + join + ON + " " + prev + ".id = " + join + "." + singular(prev) + "Id" +
                     includeJoin(join)
                break
            }
            return cmd
        }


        /*
         *  Common find implementation. See find/findAll for doc.
         */
        static private function innerFind(key: Object, options: Object = null): Array {
            let cmd: String
            let columns: Array
            let from: String
            let conditions: String
            let where: String

            //  TODO - BUG options: Object = {} doesn't work in args above
            if (options == null) {
                options = {}
            }

            if (options.columns) {
                columns = options.columns
                /*
                 *  Qualify "id" so it won't clash when doing joins. If the "click" option is specified, 
                 *  must have ID in the data
                 */
                let index: Number = columns.indexOf("id")
                if (index >= 0) {
                    columns[index] = _tableName + ".id"
                } else if (!columns.contains(_tableName + ".id")) {
                    columns.insert(0, _tableName + ".id")
                }
            } else {
                columns =  "*"
            }

            conditions = ""
            from = ""
            where = false

            if (options.from) {
                from = options.from

            } else if (options.include) {
                //  TODO - incomplete
                from = _tableName + " " + includeJoins(options.include)

            } else {
                from = _tableName
                
                /*
                 *  Join other belonging models if a key has not been provided
                 */
                if (_belongsTo && options.joinBelongs != false) {
                    conditions = " ON "
                    for each (let owner in _belongsTo) {
                        from += " INNER JOIN " + owner._tableName
                    }
                    for each (let owner in _belongsTo) {
                        let tname = Reflect(owner).name
                        tname = tname[0].toLower() + tname.slice(1) + "Id"
                        conditions += _tableName + "." + tname + " = " + owner._tableName + "." + owner._keyName + " AND "
                    }
                }
                if (options.joins) {
                    if (conditions == "") {
                        conditions = " ON "
                    }
                    let parts: Array = options.joins.split(" ON ")
                    from += " " + parts[0]
                    if (parts.length > 1) {
                        conditions += parts[1] + " AND "
                    }
                }
            }
            conditions = conditions.trim(" AND ")

            if (options.conditions) {
                where = true
                conditions += " WHERE "
                if (options.conditions is Array) {
                    for each (cond in options.conditions) {
                        conditions += cond + " " + " OR "
                    }
                    conditions = conditions.trim(" OR ")

                } else if (options.conditions is Object) {
                    for (field in options.conditions) {
                        conditions += field + " = '" + options.conditions[field] + "' " + " AND "
                    }
                }
                conditions = conditions.trim(" AND ")

            } else {
                from = from.trim(" AND ")
            }

            if (key || options.key) {
                if (!where) {
                    conditions += " WHERE "
                    where = true
                } else {
                    conditions += " AND "
                }
                conditions += (_tableName + "." + _keyName + " = ") + ((key) ? key : options.key)
            }

            cmd = "SELECT " + columns + " FROM " + from + conditions
            if (options.group) {
                cmd += " GROUP BY " + options.group
            }
            if (options.order) {
                cmd += " ORDER BY " + options.order
            }
            if (options.limit) {
                cmd += " LIMIT " + options.limit
            }
            if (options.offset) {
                cmd += " OFFSET " + options.offset
            }

            cmd += ";"
            log("find", cmd)

            let db = getDb()
            if (db == null) {
                throw new Error("Database connection has not yet been established")
            }

            let results: Array
            try {
                results = db.query(cmd)
            }
            catch (e) {
                throw e
            }

            // dump(results)

/*
            for each (rec in results) {
                //  TODO - when we have catchalls, these should be loaded only on demand access
                if (_belongsTo) {
                    for each (modelType in _belongsTo) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId])
                    }
                }
                if (hasOne) {
                    for each (modelType in hasOne) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId])
                    }
                }
                if (_hasMany) {
                    for each (modelType in _hasMany) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId], true)
                    }
                }
            }
*/
            return results
        }


        /**
         *  Get the database connection for this record class
         *  @returns Database instance object created via new $Database
         */
        static function getDb(): Database {
            if (_db == null) {
                return Database.defaultDatabase
            } else {
                return _db
            }
        }


        /**
         *  Return the column names for the table
         *  @returns an array containing the names of the database columns. This corresponds to the set of properties
         *      that will be created when a row is read using $find.
         */
        static function get columnNames(): Array { 
            if (_columnNames == null) {
                getSchema()
            }
            let result: Array = []
            for (name in _columnNames) {
                result.append(name)
            }
            return result
        }


        /**
         *  Return the column names for the record
         *  @returns an array containing the Pascal case names of the database columns. The names have the first letter
         *      capitalized. 
         */
        static function get columnTitles(): Array { 
            if (_columnNames == null) {
                getSchema()
            }
            let result: Array = []
            for each (title in _columnNames) {
                result.append(title)
            }
            return result
        }


        /**
         *  Get the key name for this record
         */
        static function getKeyName(): String {
            return _keyName
        }


        //  TODO - should this be a get numRows?
        /**
         *  Return the number of rows in the table
         */
        static function getNumRows(): Number {
            let cmd: String = "SELECT COUNT(*) FROM " + _tableName + " WHERE " + _keyName + " <> '';"
            log("getNumRows", cmd)
            let grid: Array = getDb().query(cmd)
            return grid[0]["COUNT(*)"]
        }


        /*
         *  Read the table schema
         */
        private static function getSchema(): Void {
            if (_className == null) {
                throw new Error("Model is not initialized. Must call Model.setup() first")
            }
            if (getDb() == undefined) {
                throw new Error("Can't get schema, database connection has not yet been established")
            }

            let sql: String = 'PRAGMA table_info("' + _tableName + '");'
            log("schema", sql)

            let grid: Array = getDb().query(sql)
            _columnNames = {}
            _columnTypes = {}
            _sqlColumnTypes = {}
            for each (let row in grid) {
                let name: String = row["name"]
                _columnNames[name] = name.toPascal()
                _sqlColumnTypes[name] = row.type.toLower()
                _columnTypes[name] = mapSqlTypeToEjs(row.type.toLower())
                //  Others "dflt_value", "notnull" "pk"
            }
        }


        /**
         *  Get the associated name for this record
         *  @returns the database table name backing this record class. Normally this is simply a  pluralized class name. 
         */
        static function getTableName(): String {
            return _tableName
        }


        //  TODO - options are ignored
        /**
         *  Define a containment relationship to another model class. When using "hasMany" on another model, it means 
         *  that other model has a foreign key reference to this class and this class can "contain" many instances of 
         *  the other.
         *  @param model Model class that is contained by this class. 
         *  @option things Model object that is posessed by this. 
         *  @option through String Class name which mediates the many to many relationship
         *  @option foreignKey Key name for the foreign key
         */
        static function hasMany(model: Object, options: Object = null): Void {
            //  TODO - incomplete
            if (_hasMany == undefined) {
                _hasMany = [model]
            } else {
                _hasMany.append(model)
            }
        }


        /**
         *  Define a containment relationship to another model class. When using "hasAndBelongsToMany" on another model, it 
         *  means that other models have a foreign key reference to this class and this class can "contain" many instances 
         *  of the other models.
         *  @option thing Model 
         *  @option foreignKey Key name for the foreign key
         *  @option through String Class name which mediates the many to many relationship
         *  @option joinTable
         */
        static function hasAndBelongsToMany(thing: Object, options: Object = null): Void {
            belongs(thing, options)
            hasMany(thing, options)
        }


        //  TODO - options are ignored
        /**
         *  Define a containment relationship to another model class. When using "hasOne" on another model, 
         *  it means that other model has a foreign key reference to this class and this class can "contain" 
         *  only one instance of the other.
         *  @param model Model class that is contained by this class. 
         *  @option thing Model that is posessed by this.
         *  @option foreignKey Key name for the foreign key
         *  @option as String 
         */
        static function hasOne(model: Object, options: Object = null): Void {
            //  TODO - incomplete
            if (_hasOne == undefined) {
                _hasOne = [model]
            } else {
                _hasOne.append(model)
            }
        }


        private static function loadReference(model: Record, key: String, hasMany: Boolean = false): Object {
            let data: Array = model.innerFind(key)
            if (hasMany) {
                let result: Array = new Array
                for each (item in data) {
                    result.append(model.createRecord(data[0]))
                }
                return result
                
            } else {
                if (data.length > 0) {
                    return model.createRecord(data[0])
                }
            }
            return null
        }


        //  TODO - should log be pushed into database.query -- yes
        private static function log(where: String, cmd: String): Void {
            if (_traceSql) {
                print(where + " SQL: " + cmd)
            }
        }


        //  TODO - unused
        private static function logResult(data: Object): Void {
            dump(data)
        }


        private static function mapSqlTypeToEjs(sqlType: String): String {
            sqlType = sqlType.replace(/\(.*/, "")
            let ejsType: String = Database.SqliteToEjs[sqlType]
            if (ejsType == undefined) {
                throw new Error("Unsupported SQL type: \"" + sqlType + "\"")
            }
            return ejsType
        }


        //  TODO - need to have an instance method to remove a record from the database
        /**
         *  Remove records from the database
         *  @param keys Set of keys identifying the records to remove
         */
        static function remove(...keys): Void {
            for each (let key: Object in keys) {
                let cmd: String = "DELETE FROM " + _tableName + " WHERE " + _keyName + " = " + key + ";"
                log("remove", cmd)
                getDb().query(cmd)
            }
        }


        /**
         *  Set the database connection for this record class
         *  @param database Database instance object created via new $Database
         */
        static function setDb(dbase: Database): Void {
            _db = dbase
        }


        /**
         *  Initialize the model
         *  @param database Reference to the database object.
         */
        static function setup(database: Database = null): Void {
            _db = database
            _className = Reflect(this).name
            _tableName = pluralize(_className)
            _foreignId = _className.toCamel() + "Id"
            _keyName = "id"
        }


        /**
         *  Set the associated table name for this record
         *  @param name Name of the database table to backup the record class.
         */
        static function setTableName(name: String): Void {
            _tableName = name
        }


        /**
         *  Set the key name for this record
         */
        static function setKeyName(value: String): Void {
            _keyName = value
        }


        /**
         *  Run an SQL statement and return selected records.
         *  @param sql SQL command to issue
         *  @returns An array of objects. Each object represents a matching row with fields for each column.
         */
        static function sql(cmd: String, count: Number = null): Array {
            cmd = "SELECT " + cmd + ";"
            log("select", cmd)
            return getDb().query(cmd)
        }


        /**
         *  Trace SQL statements. Control whether trace is enabled for the actual SQL statements issued against the database.
         *  @param on If true, display each SQL statement to the log
         */
        static function trace(on: Boolean): void {
            _traceSql = on
        }


        static function validateFormat(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkFormat, fields, options])
        }


        static function validateNumber(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkNumber, fields, options])
        }


        static function validatePresence(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkPresent, fields, options])
        }


        static function validateUnique(fields: Object, options: Object = null) {
            _validations.append([checkUnique, fields, options])
        }


        static var ErrorMessages = {
            accepted: "must be accepted",
            blank: "can't be blank",
            confirmation: "doesn't match confirmation",
            empty: "can't be empty",
            invalid: "is invalid",
            missing: "is missing",
            notNumber: "is not a number",
            notUnique: "is not unique",
            taken: "already taken",
            tooLong: "is too long",
            tooShort: "is too short",
            wrongLength: "wrong length",
            wrongFormat: "wrong format",
        }


        private static function checkFormat(thisObj: Object, field: String, value, options): Void {
            if (! RegExp(options.format).test(value)) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.wrongFormat
            }
        }


        private static function checkNumber(thisObj: Object, field: String, value, options): Void {
            if (! RegExp(/^[0-9]+$/).test(value)) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.notNumber
            }
        }


        private static function checkPresent(thisObj: Object, field: String, value, options): Void {
            if (value == undefined) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.missing
            } else if (value.length == 0 || value.trim() == "" && thisObj._errors[field] == undefined) {
                thisObj._errors[field] = ErrorMessages.blank
            }
        }


        private static function checkUnique(thisObj: Object, field: String, value, options): Void {
            if (thisObj._keyValue) {
                grid = findWhere(field + ' = "' + value + '" AND id <> ' + thisObj._keyValue)
            } else {
                grid = findWhere(field + ' = "' + value + '"')
            }
            if (grid.length > 0) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.notUnique
            }
        }

        function error(field: String, msg: String): Void {
            if (!field) {
                field = ""
            }
            _errors[field] = msg
        }

        function validateModel(): Boolean {
            _errors = {}
            if (_validations) {
                for each (let validation: String in _validations) {
                    check = validation[0]
                    fields = validation[1]
                    options = validation[2]
                    if (fields is Array) {
                        for each (let field in fields) {
                            if (_errors[field]) {
                                continue
                            }
                            check(this, field, this[field], options)
                        }
                    } else {
                        check(this, fields, this[fields], options)
                    }
                }
            }
            self = Reflect(this).type
            if (self["validate"]) {
                self["validate"].call(this)
            }
            if (_errors.length == 0) {
                coerceTypes()
            }
            return _errors.length == 0
        }


        function getErrors(): Array {
            return _errors
        }


        function hasError(field: String = null): Boolean {
            if (field) {
                return (_errors && _errors[field])
            }
            return _errors && _errors.length > 0
        }


        function getFieldType(field: String): String {
            if (_columnNames == null) {
                getSchema()
            }
            return Database.SqliteToDatatype[_sqlColumnTypes[field]]
        }


        private function coerceTypes(): Void {
            for (let field: String in this) {
                if (_columnTypes[field] == Reflect(this[field]).type) {
                    continue
                }
                let value: String = this[field]
                switch (_columnTypes[field]) {
                case Boolean:
                    if (value is String) {
                        this[field] = (value.toLower() == "true")
                    } else if (value is Number) {
                        this[field] = (value == 1)
                    } else {
                        this[field] = value cast Boolean
                    }
                    this[field] = (this[field]) ? true : false
                    break

                case Date:
                    this[field] = new Date(value)
                    break

                case Number:
                    this[field] = this[field] cast Number
                    break
                }
            }
        }

/*
        static function selectCount(sql: String, count: Number): Object
        static function removeAll(): Void {}
        static function average(column: String): Number {}
        static function maximum(column: String): Number {}
        static function minimum(column: String): Number {}
        static function sum(column: String): Number {}
        static function count(column: String): Number {}
*/

    }


    /*
     *  Internal class to demand load model references and collections
     *
FUTURE
    class DemandLoader {
        var cached: Record
        var hasMany: Boolean
        var key: Number
        var loaded: Boolean
        var model: Record


        function DemandLoader(model: Record, key: Number, hasMany: Boolean = false) {
            this.model = model
            this.key = key
            this.hasMany = hasMany
        }


        function load(): Record {
            if (!loaded) {
                data = this.innerFind(key)
                if (hasMany) {
                    cached = new Array
                    for each (item in data) {
                        cached.append(model.createRecord(data[0]))
                    }
                } else {
                    if (data.length > 0) {
                        cached = model.createRecord(data[0])
                    } else {
                        cached = null
                    }
                }
            }
            return cached
        }


        function reload(): Record {
            loaded = false
            return load
        }
    }
*/

    //  TODO - need real pluralize() singularize()
    function pluralize(name: String): String {
        var s: String = name + "s"
        return s.toPascal()
    }

}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/db/Record.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/events/Dispatcher.es"
 */
/************************************************************************/

/*
 *	Dispatcher.es -- Event dispatcher target.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.events {

    /*
     *  TODO - this is a temporary implementation that does not integrate with the MPR event queue mechanism.
     */
	/**
	 *	The Dispatcher class supports the registration of listeners who want notification of events of interest.
	 *
	 *	@example
	 *
	 *		class Shape {
	 *			public var events: Dispatcher = new Dispatcher
	 *		}
	 *
	 *		function callback(e: Event) {
	 *		}
	 *
	 *		var s : Shape = new Shape
	 *		s.events.addListener(callback)
	 *
	 *		s.events.dispatch(new Event("Great Event"))
	 */
	class Dispatcher {
		var events: Object

        use default namespace public

        /**
         *  Construct a new event Dispatcher object
         */
		function Dispatcher() {
			events = new Object
		}


		/**
		 *	Add a listener function for events.
		 *	@param callback Function to call when the event is received.
		 *	@param eventType Event class to listen for. The listener will receive events of this event class or any 
		 *		of its subclasses. Defaults to Event.
		 */
		function addListener(callback: Function, eventType: Type = Event): Void {
            var name = Reflect(eventType).name
			var listeners : Array

			listeners = events[name]

			if (listeners == undefined) {
				listeners = events[name] = new Array

			} else {
				for each (var e: Endpoint in listeners) {
					if (e.callback == callback && e.eventType == eventType) {
						return
					}
				}
			}
			listeners.append(new Endpoint(callback, eventType))
		}


		/**
		 *	Dispatch an event to the registered listeners. This is called by the class owning the event dispatcher.
		 *	@param event Event instance to send to the listeners.
		 */
		function dispatch(event: Event): Void {
			var eventListeners : Array
            var name = Reflect(event).typeName

			eventListeners = events[name]
			if (eventListeners != undefined) {
				for each (var e: Endpoint in eventListeners) {
					if (event is e.eventType) {
						e.callback(event)
					}
				}
			}
		}


		/**
		 *	Remove a registered listener.
		 *	@param eventType Event class used when adding the listener.
		 *	@param callback Listener callback function used when adding the listener.
		 */
		function removeListener(callback: Function, eventType: Type = Event): Void {
            var name = Reflect(eventType).name
			var listeners: Array

			listeners = events[name]
			if (listeners == undefined) {
				return
			}

			for (let i in listeners) {
				var e: Endpoint = listeners[i]
				if (e.callback == callback && e.eventType == eventType) {
					listeners.remove(i, i + 1)
				}
			}
		}
	}


    /**
     *  Reserved class for use by Event
     */
	internal class Endpoint {
		public var	callback: Function
		public var	eventType: Type

		function Endpoint(callback: Function, eventType: Type) {
			this.callback = callback
			this.eventType = eventType
		}
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/events/Dispatcher.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/events/Event.es"
 */
/************************************************************************/

/*
 *	Event.es -- Event class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.events {

	/**
	 *	The Event class encapsulates information pertaining to a system or application event. Applications typically
	 *	subclass Event to add custom event data if required. Events are initiated via the EventTarget class and are
	 *	routed to listening functions via a system event queue.
	 *
	 *	Example:
	 *	
	 *		class UpdateEvent extends Event {
	 *		}
	 *
	 *		obj.events.dispatch(new UpdateEvent())
	 */

	class Event {

        use default namespace public

        /**
         *  Low priority constant for use with the Event() constructor method.
         */
		static const	PRI_LOW: Number		= 25;


        /**
         *  Normal priority constant for use with the Event() constructor method.
         */
		static const	PRI_NORMAL: Number	= 50;


        /**
         *  High priority constant for use with the Event() constructor method.
         */
		static const	PRI_HIGH: Number	= 75;


		/**
		 *	Whether the event will bubble up to the listeners parent
		 */
		var bubbles: Boolean


		/**
		 *	Event data associated with the Event. When Events are created, the constructor optionally takes an arbitrary 
		 *	object data reference.
		 */
		var data: Object


		/**
		 *	Time the event was created. The Event constructor will automatically set the timestamp to the current time.  
		 */
		var timestamp: Date


		/**
		 *	Event priority. Priorities are 0-99. Zero is the highest priority and 50 is normal. Use the priority 
		 *	symbolic constants PRI_LOW, PRI_NORMAL or PRI_HIGH.
		 */
		var priority: Number


		/**
		 *	Constructor for Event. Create a new Event object.
		 *	@param data Arbitrary object to associate with the event.
		 *	@param bubbles Bubble the event to the listener's parent if true. Not currently implemented.
		 *	@param priority Event priority.
		 */
		function Event(data: Object = null, bubbles: Boolean = false, priority: Number = PRI_NORMAL) {
			this.timestamp = new Date
			this.data = data
			this.priority = priority
			this.bubbles = bubbles
		}

		//	TODO - BUG - not overriding native method
		override function toString(): String {
			return "[Event: " +  Reflect(this).typeName + "]"
		}
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/events/Event.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/events/Timer.es"
 */
/************************************************************************/

/*
 *	Timer.es -- Timer Services
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.events {

	/**
	 *	Timers manage the execution of functions at some point in the future. Timers run repeatedly until stopped by 
	 *	calling the @stop method and are scheduled with a granularity of 1 millisecond. However, many systems are not 
	 *	capable of supporting this granularity and make only best efforts to schedule events at the desired time.
	 *
	 *	@Example
	 *		function callback(e: Event) {
	 *		}
	 *		new Timer(200, callback)
	 *	and
	 *		new Timer(200, function (e) { print(e); })
	 */
	native class Timer {

        use default namespace public

		/**
		 *	Constructor for Timer. The timer is will not be called until @start is called.
		 *	@param callback Function to invoke when the timer is due.
		 *	@param period Time period in milliseconds between invocations of the callback
		 *	@param drift Set the timers drift setting. See @drift.
		 */
		native function Timer(period: Number, callback: Function, drift: Boolean = true)


		/**
		 *	Get the current drift setting.
		 *	@return True if the timer is allowed to drift its execution time due to other system events.
		 */
		native function get drift(): Boolean


		/**
		 *	Set the timer drift setting.
		 *	If drift is false, reschedule the timer so that the time period between callback start times does not drift 
		 *	and is best-efforts equal to the timer reschedule period. The timer subsystem will delay other low priority
		 *	events or timers, with drift equal to true, if necessary to ensure non-drifting timers are scheduled exactly. 
		 *	Setting drift to true will schedule the timer so that the time between the end of the callback and the 
		 *	start of the next callback invocation is equal to the period. 
		 *	@param enable If true, allow the timer to drift
		 */
		native function set drift(enable: Boolean): Void


		/**
		 *	Get the timer interval period in milliseconds.
		 */
		native function get period(): Number


		/**
		 *	Set the timer period and reschedule the timer.
		 *	@param period New time in milliseconds between timer invocations.
		 */
		native function set period(period: Number): Void


		/**
		 *	Restart a stopped timer. Once running, the callback function will be invoked every @period milliseconds 
		 *	according to the @drift setting. If the timer is already stopped, this function has no effect
		 */
		native function restart(): Void
		

		/**
		 *	Stop a timer running. Once stopped a timer can be restarted by calling @start.
		 */
		native function stop(): Void
	}


	/**
	 *	Timer event
	 */
	class TimerEvent extends Event {
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/events/Timer.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/BinaryStream.es"
 */
/************************************************************************/

/*
 *  BinaryStream.es -- BinaryStream class. This class is a filter or endpoint stream to encode and decode binary types.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    /**
     *  BinaryStreams encode and decode various objects onto streams. A BinaryStream may be stacked atop an underlying stream
     *  provider such as File, Http or Socket. The underlying stream must be in sync mode.
     *  @spec ejs-11
     */
    class BinaryStream implements Stream {

        use default namespace public

		/**
		 *	Big endian byte order 
         */
        static const BigEndian: Number = ByteArray.BigEndian

		/**
		 *	Little endian byte order 
         */
        static const LittleEndian: Number = ByteArray.LittleEndian

        /*
         *  Data input and output buffers. The buffers are used to marshall the data for encoding and decoding. The inbuf 
         *  also  hold excess input data. The outbuf is only used to encode data -- no buffering occurs.
         */
        private var inbuf: ByteArray
        private var outbuf: ByteArray
        private var nextStream: Stream

        /**
         *  Create a new BinaryStream
         *  @param stream Optional stream to stack upon.
         */
        function BinaryStream(stream: Stream = null) {
            nextStream = stream
            inbuf = new ByteArray
            outbuf = new ByteArray

            /*
             *  Setup the input and output callbacks. These are invoked to get/put daa.
             */
            inbuf.input = function (buffer: ByteArray) {
                nextStream.read(buffer)
            }

            outbuf.output = function (buffer: ByteArray) {
                count = nextStream.write(buffer)
                buffer.readPosition += count
                buffer.reset()
            }
        }

        /**
         *  Close the input stream and free up all associated resources.
         *  @param graceful if true, then close the socket gracefully after writing all pending data.
         */
        function close(graceful: Boolean = 0): void {
            flush()
            nextStream.close()
        }

        /**
         *  Determine if the system is using little endian byte ordering
         *  @return An endian encoding constant. Either LittleEndian or BigEndian
         */
        function get endian(): Number
            inbuf.endian

        /**
         *  Set the system encoding to little or big endian.
         *  @param value Set to true for little endian encoding or false for big endian.
         */
        function set endian(value: Number): Void {
            if (value != BigEndian && value != LittleEndian) {
                throw new ArgError("Bad endian value")
            }
            inbuf.endian = value
            outbuf.endian = value
        }

        /**
         *  Flush the stream and all stacked streams and underlying data source/sinks.
         */
        function flush(): void {
            inbuf.flush()
            outbuf.flush()
            nextStream.flush()
        }

        /**
         *  Read data from the stream. 
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data.
         *  @param count Number of bytes to read. 
         *  @returns a count of the bytes actually read.
         *  @throws IOError if an I/O error occurs.
         */
        function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
            inbuf.read(buffer, offset, count)

        /**
         *  Read a boolean from the stream.
         *  @returns a boolean
         *  @throws IOError if an I/O error occurs.
         */
        function readBoolean(): Boolean
            inbuf.readBoolean()

        /**
         *  Read a byte from the stream.
         *  @returns a byte
         *  @throws IOError if an I/O error occurs.
         */
        function readByte(): Number
            inbuf.readByte()

        /**
         *  Read data from the stream into a byte array.
         *  @returns a new byte array with the available data
         *  @throws IOError if an I/O error occurs.
         */
        function readByteArray(count: Number = -1): ByteArray
            inbuf.readByteArray(count)

        /**
         *  Read a date from the stream.
         *  @returns a date
         *  @throws IOError if an I/O error occurs.
         */
        function readDate(): Date
            inbuf.readDate()

        /**
         *  Read a double from the stream. The data will be decoded according to the encoding property.
         *  @returns a double
         *  @throws IOError if an I/O error occurs.
         */
        function readDouble(): Date
            inbuf.readDate()

        /**
         *  Read a 32-bit integer from the stream. The data will be decoded according to the encoding property.
         *  @returns an 32-bitinteger
         *  @throws IOError if an I/O error occurs.
         */
        function readInteger(): Number
            inbuf.readInteger()

        /**
         *  Read a 64-bit long from the stream.The data will be decoded according to the encoding property.
         *  @returns a 64-bit long number
         *  @throws IOError if an I/O error occurs.
         */
        function readLong(): Number
            inbuf.readInteger()

        /**
         *  Read a UTF-8 string from the stream. 
         *  @param count of bytes to read. Returns the entire stream contents if count is -1.
         *  @returns a string
         *  @throws IOError if an I/O error occurs.
         */
        function readString(count: Number = -1): String 
            inbuf.readString(count)

        /**
         *  Read an XML document from the stream. This assumes the XML document will be the only data until EOF.
         *  @returns an XML document
         *  @throws IOError if an I/O error occurs.
         */
        function readXML(): XML {
            var data: String = ""
            while (1) {
                var s: String = inbuf.readString()
                if (s.length == 0) {
                    break
                }
                data += s
            }
            return new XML(data)
        }


        /**
         *  Write data to the stream. Write intelligently encodes various @data types onto the stream and will encode data 
         *  in a portable cross-platform manner according to the setting of the endian ByteStream property. If data is an 
         *  array, each element of the array will be written. The write call blocks until the underlying stream or endpoint 
         *  absorbes all the data. 
         *  @param data Data to write. The ByteStream class intelligently encodes various data types according to the
         *  current setting of the @endian BinaryStream property. 
         *  @returns The total number of elements that were written.
         *  @throws IOError if there is an I/O error.
         */
        function write(...items): Number {
            let count: Number = 0
            for each (i in items) {
                count += outbuf.write(i)
            }
            return count
        }

        //  TODO - should these routines return a count of bytes?  YES
        /**
         *  Write a byte to the array. Data is written to the current write $position pointer.
         *  @param data Data to write
         */
        function writeByte(data: Number): Void 
            outbuf.writeByte(outbuf)


        /**
         *  Write a short to the array. Data is written to the current write $position pointer.
         *  @param data Data to write
         */
        function writeShort(data: Number): Void
            outbuf.writeShort(data)


        /**
         *  Write a double to the array. Data is written to the current write $position pointer.
         *  @param data Data to write
         */
        function writeDouble(data: Number): Void
            outbuf.writeDouble(data)


        /**
         *  Write a 32-bit integer to the array. Data is written to the current write $position pointer.
         *  @param data Data to write
         */
        function writeInteger(data: Number): Void
            outbuf.writeInteger(data)


        /**
         *  Write a 64 bit long integer to the array. Data is written to the current write $position pointer.
         *  @param data Data to write
         */
        function writeLong(data: Number): Void
            outbuf.writeLong(data)
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/BinaryStream.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/File.es"
 */
/************************************************************************/

/*
 *  File.es -- File I/O class. Do file I/O and manage files.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    /**
     *  The File class provides a foundation of I/O services to interact with physical files.
     *  Each File object represents a single file, a named path to data stored in non-volatile memory. A File object 
     *  provides methods for creating, opening, reading, writing and deleting a file, and for accessing and modifying 
     *  information about the file.
     *  @spec ejs-11
     */
    native class File implements Stream {

        use default namespace public

        /**
         *  File access mode representing a Closed file.
         */
        static const Closed: Number     = 0x0

        /**
         *  File access mode representing an Opened file.
         */
        static const Open: Number       = 0x1

        /**
         *  File access mode representing an opened file for read access.
         */
        static const Read: Number       = 0x2

        /**
         *  File access mode representing an opened file for write access
         */
        static const Write: Number      = 0x4   

        /**
         *  File access mode representing a file opened for append write access.
         */
        static const Append: Number     = 0x8

        /**
         *  File access mode where the file will be re-created when opened.
         */
        static const Create: Number     = 0x10

        /**
         *  File access mode where the file will be truncated when opened.
         */
        static const Truncate: Number   = 0x20


        /**
         *  Create a new File object and set the file object's path.
         *  @param path the name of the file to associate with this file object.
         */
        native function File(path: String)


		//	TODO - need isAbsolute and isRelative functions
        /**
         *  Return an absolute path name for the file.
         *  @return a string containing an absolute file name relative to the file system's root directory. 
         *  The file name is canonicalized such that multiple directory separators and ".." segments are removed.
         *  @see relativePath
         */
        native function get absolutePath(): String

        /**
         *  Get the base name of a file. Returns the base name portion of a file name. The base name portion is the 
         *  trailing portion without any directory elements.
         *  @return A string containing the base name portion of the file name.
         */
        native function get basename(): String

        /**
         *  Close the input stream and free up all associated resources.
         *  @param graceful if true, then close the file gracefully after writing all pending data.
         */
        native function close(graceful: Boolean = true): void 

        /**
         *  Copy a file. If the destination file already exists, the old copy will be overwritten as part of the copy 
		 *		operation.
         *  @param toPath New destination file path name.
         *  @throws IOError if the copy is not successful.
         */
        native function copy(toPath: String): void
        
        /*
         *  Create a temporary file. Creates a new, uniquely named temporary file.
         *  @param directory Directory in which to create the temp file.
         *  @returns a closed File object after creating an empty temporary file.
         */
        native static function createTempFile(directory: String = null): File

        /**
         *  Get when the file was created.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get created(): Date 

        /**
         *  Get the directory name portion of a file. The dirname name portion is the leading portion including all 
         *  directory elements and excluding the base name. On some systems, it will include a drive specifier.
         *  @return A string containing the directory name portion of the file name.
         */
        native function get dirname(): String

        /**
         *  Test to see if this file exists.
         *  @return True if the file exists.
         */
        native function get exists(): Boolean 

        /**
         *  Get the file extension portion of the file name.
         *  @return String containing the file extension
         */
        native function get extension(): String 

        /**
         *  Flush the stream and the underlying file data. Will block while flushing. Note: may complete before
         *  the data is actually written to disk.
         */
        native function flush(): void 

        /**
         *  Return the free space in the file system.
         *  @return The number of 1M blocks (1024 * 1024 bytes) of free space in the file system.
         */
        native function freeSpace(path: String = null): Number
        
        /**
         *  Get an iterator for this file to be used by "for (v in files)". Return the seek positions for each byte.
         *  @param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
         *  @return An iterator object.
         *  @spec ejs-11
         */
        override iterator native function get(deep: Boolean = false): Iterator

        //  TODO - good to add ability to do a regexp on the path or a filter function
        //  TODO - good to add ** to go recursively to any depth
        /**
         *  Get a list of files in a directory. The returned array contains the base file name portion only.
         *  @param path Directory path to enumerate.
         *  @param enumDirs If set to true, then dirList will include sub-directories in the returned list of files.
         *  @return An Array of strings containing the filenames in the directory.
         */
        native function getFiles(enumDirs: Boolean = false): Array 

        /**
         *  Get an iterator for this file to be used by "for each (v in obj)". Return each byte of the file in turn.
         *  @param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
         *  @return An iterator object.
         *  @spec ejs-11
         */
        override iterator native function getValues(deep: Boolean = false): Iterator

        /**
         *  Get the file contents as an array of lines. Each line is a string. This is a static method that opens the file, 
         *  reads the contents and closes the file.
         *  @param path the name of the file to read.
         *  @return An array of strings.
         *  @throws IOError if the file cannot be read
         */
        native static function getBytes(path: String): ByteArray 

        //  TODO - missing a way to get lines for an already open file
        /**
         *  Get the file contents as an array of lines. Each line is a string. This is a static method that 
         *  opens the file, reads the contents and closes the file.
         *  @param path the name of the file to read.
         *  @return An array of strings.
         *  @throws IOError if the file cannot be read
         */
        native static function getLines(path: String, encoding: String = App.UTF_8): Array 


        /**
         *  Get the file contents as a string. This is a static method that opens the file, reads the contents and 
         *  closes the file.
         *  @param path the name of the file to read.
         *  @return A string.
         *  @throws IOError if the file cannot be read
         */
        native static function getString(path: String, encoding: String = App.UTF_8): String 


        /**
         *  Get the file contents as an XML object.  This is a static method that opens the file, reads the contents 
         *  and closes the file.
         *  @param path the name of the file to read.
         *  @return An XML object
         *  @throws IOError if the file cannot be read
         */
        native static function getXML(path: String): XML 


        /**
         *  Determine if the file path has a drive spec (C:) in it's name. Only relevant on Windows like systems.
         *  @return True if the file path has a drive spec
         */
        native function get hasDriveSpec(): Boolean 


        /**
         *  Determine if the file name is a directory
         *  @return true if the file is a directory
         */
        native function get isDir(): Boolean


        /**
         *  Determine if the file is currently open for reading or writing
         *  @return true if the file is currently open via @open or @create
         */
        native function get isOpen(): Boolean


        /**
         *  Determine if the file name is a regular file
         *  @return true if the file is a regular file and not a directory
         */
        native function get isRegular(): Boolean


        /**
         *  Get when the file was last accessed.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get lastAccess(): Date 


        /**
         *  Get the length of the file associated with this File object.
         *  @return The number of bytes in the file or -1 if length determination failed.
         */
        override native function get length(): Number 


        /**
         *  Set the length of the file associated with this File object.
         *  @param value the new length of the file
         *  @throws IOError if the file's length cannot be changed
         */
        # FUTURE
        intrinsic native function set length(value: Number): Void 


        /**
         *  Make a new directory. Makes a new directory and all required intervening directories. If the directory 
         *  already exists, the function returns without throwing an exception.
         *  @param path Filename path to use.
         *  @throws IOError if the directory cannot be created.
         */
        native function makeDir(permissions: Number = 0755): void
        

        /**
         *  Get the file access mode.
         *  @return file access mode with values ored from: Read, Write, Append, Create, Open, Truncate 
         */ 
        native function get mode(): Number


        /**
         *  Get when the file was created or last modified.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get modified(): Date 


        /**
         *  Get the name of the file associated with this File object.
         *  @return The name of the file or null if there is no associated file.
         */
        native function get name(): String 


        //  TODO - this needs a path arg to define the file system
        /**
         *  Return the new line characters
         *  @return the new line delimiting characters Usually "\n" or "\r\n".
         */
        native static function get newline(): String 


        /**
         *  Set the new line characters
         *  @param terminator the new line termination characters Usually "\n" or "\r\n"
         */
        native static function set newline(terminator: String): Void


        /**
         *  Open a file using the current file name. This method requires a file instance.
         *  @param name The (optional) name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate.
         *  Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @throws IOError if the path or file cannot be created.
         */
        native function open(mode: Number = Read, permissions: Number = 0644): void


        /**
         *  Open a file and return a Stream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. Defaults 
         *  to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a File object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openFileStream(filename: String, mode: Number = Read, permissions: Number = 0644): File {
            var file: File

            file = new File(filename)
            file.open(mode, permissions)
            return file
        }


        /**
         *  Open a file and return a TextStream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. 
         *      Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a TextStream object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openTextStream(filename: String, mode: Number = Read, permissions: Number = 0644): TextStream {
            var file: File = new File(filename)
            file.open(mode, permissions)
            return new TextStream(file)
        }


        /**
         *  Open a file and return a BinaryStream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. 
         *      Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a BinaryStream object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openBinaryStream(filename: String, mode: Number = Read, permissions: Number = 0644): BinaryStream {
            var file: File = new File(filename)
            file.open(mode, permissions)
            return new BinaryStream(file)
        }


        //  TODO - this needs a path arg to define the file system
        /**
         *  Return the path segment delimiter
         *  @return the path segment delimiter. Usually "/" or "\\"
         */
        native static function get pathDelimiter(): String 


        //  TODO - remove this API. Should be defined internally by the file system.
        /**
         *  Set the path segment delimiter
         *  @param delim the new path segment delimiter. Usually "/" or "\\"
         */
        native static function set pathDelimiter(delim: String): Void 


        /**
         *  Get the parent directory of the absolute path of the file.
         *  @return the parent directory
         */
        native function get parent(): String


        /**
         *  Get the file security permissions.
         *  @return the file permission mask. This is a POSIX file creation mask.
         */
        native function get permissions(): Number


        /**
         *  Set the file security permissions. 
         *  @return the file permission mask. This is a POSIX file creation mask.
         */
        native function set permissions(mask: Number): void


        /**
         *  Get the current I/O position in the file.
         *  @returns the current read / write position in the file.
         *  @throws IOError if the seek failed.
         */
        native function get position(): Number


        /**
         *  Seek to a new location in the file and set the File marker to a new read/write position.
         *  @param number Location in the file to seek to. Set the position to zero to reset the position to the beginning 
         *  of the file. Set the position to a negative number to seek relative to the end of the file (-1 positions 
         *  at the end of file).
         *  @throws IOError if the seek failed.
         */
        native function set position(value: Number): void


//  TODO - having permissings in the middle doesn't work well
        /**
         *  Put the file contents.  This is a static method that opens the file, writes the contents and closes the file.
         *  @param path the name of the file to write.
         *  @param data to write to the file.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return An XML object
         *  @throws IOError if the file cannot be written
         */
        native static function put(path: String, permissions: Number, ...args): void 


        /**
         *  Return a relative path name for the file.
         *  @return a string containing a file path name relative to the application's current working directory..
         */
        native function get relativePath()


        /**
         *  Read a block of data from a file into a byte array. This will advance the read position.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data.
         *  @param count Number of bytes to read. 
         *  @return A count of the bytes actually read.
         *  @throws IOError if the file could not be read.
         */
        native function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number


        /**
         *  Read data bytes from a file and return a byte array containing the data.
         *  @param count Number of bytes to read. 
         *  @return A byte array containing the read data
         *  @throws IOError if the file could not be read.
         */
        native function readBytes(count: Number): ByteArray

      //    TODO - need read Lines
        //  TODO - need readString();

        /**
         *  Delete the file associated with the File object. If the file is opened, it is first closed.
         *  @throws IOError if the file exists and could not be deleted.
         */
        native function remove(): void 


        //  TODO - recursive remove?
        /**
         *  Removes a directory. 
         *  @param path Filename path to remove.
         *  @throws IOError if the directory exists and cannot be removed.
         */
        native function removeDir(recursive: Boolean = false): void


        /**
         *  Rename a file. If the new file name exists it is removed before the rename.
         *  @param to New file name.
         *  @throws IOError if the original file does not exist or cannot be renamed.
         */
        native function rename(toFile: String): void
        

        /**
         *  Put the file stream into async mode and define a completion callback. When in async mode, I/O will be 
         *  non-blocking and the callback function will be invoked when the there is read data available or when 
         *  the write side can accept more data.
         *  @param callback Callback function to invoke when the pending I/O is complete. The callback is invoked 
         *  with the signature: function callback(e: Event): void.  Where e.data == stream.
         */
        native function setCallback(callback: Function): void


        /**
         *  Reduce the size of the file. 
         *  @param size The new maximum size of the file.If the truncate argument is greater than or equal to the 
         *  current file size nothing happens.
         *  @throws IOError if the truncate failed.
         */
        # FUTURE
        native function truncate(size: Number): void


        /**
         *  Return an absolute unix style path name for the file. Used on Windows when using cygwin to return
         *  a cygwin styled path without drive specifiers.
         *  @returns a string containing an absolute file name relative to the cygwin file system's root directory. The file 
         *  name is canonicalized such that drive specifiers, multiple directory separators and ".." segments are removed.
         *  @see absolutePath, relativePath
         */
        native function get unixPath(): String


        /**
         *  Write data to the file. If the stream is in sync mode, the write call blocks until the underlying stream or 
         *  endpoint absorbes all the data. If in async-mode, the call accepts whatever data can be accepted immediately 
         *  and returns a count of the elements that have been written.
         *  @param items The data argument can be ByteArrays, strings or Numbers. All other types will call serialize
         *  first before writing. Note that numbers will not be written in a cross platform manner. If that is required, use
         *  the BinaryStream class to write Numbers.
         *  @returns the number of bytes written.  
         *  @throws IOError if the file could not be written.
         */
        native function write(...items): Number
    }
}

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/File.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/Http.es"
 */
/************************************************************************/

/**
 *  Http.es -- HTTP client side communications
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    // TODO - more doc
    /**
     *  The Http object represents a Hypertext Transfer Protocol version 1.1 client connection. It is used to issue 
     *  HTTP requests and capture responses. It supports the HTTP/1.1 standard including methods for GET, POST, 
     *  PUT, DELETE, OPTIONS, and TRACE. It also supports Keep-Alive and SSL connections. 
     *  @spec ejs-11
     */
    native class Http implements Stream {

        use default namespace public

        /** HTTP status code */     static const Continue           : Number    = 100
        /** HTTP status code */     static const Ok                 : Number    = 200
        /** HTTP status code */     static const Created            : Number    = 201
        /** HTTP status code */     static const Accepted           : Number    = 202
        /** HTTP status code */     static const NotAuthoritative   : Number    = 203
        /** HTTP status code */     static const NoContent          : Number    = 204
        /** HTTP status code */     static const Reset              : Number    = 205
        /** HTTP status code */     static const Partial            : Number    = 206
        /** HTTP status code */     static const MultipleChoice     : Number    = 300
        /** HTTP status code */     static const MovedPermanently   : Number    = 301
        /** HTTP status code */     static const MovedTemporarily   : Number    = 302
        /** HTTP status code */     static const SeeOther           : Number    = 303
        /** HTTP status code */     static const NotModified        : Number    = 304
        /** HTTP status code */     static const UseProxy           : Number    = 305
        /** HTTP status code */     static const BadRequest         : Number    = 400
        /** HTTP status code */     static const Unauthorized       : Number    = 401
        /** HTTP status code */     static const PaymentRequired    : Number    = 402
        /** HTTP status code */     static const Forbidden          : Number    = 403
        /** HTTP status code */     static const NotFound           : Number    = 404
        /** HTTP status code */     static const BadMethod          : Number    = 405
        /** HTTP status code */     static const NotAccepted        : Number    = 406
        /** HTTP status code */     static const ProxyAuth          : Number    = 407
        /** HTTP status code */     static const ClientTimeout      : Number    = 408
        /** HTTP status code */     static const Conflict           : Number    = 409
        /** HTTP status code */     static const Gone               : Number    = 410
        /** HTTP status code */     static const LengthRequired     : Number    = 411
        /** HTTP status code */     static const PrecondFailed      : Number    = 412
        /** HTTP status code */     static const EntityTooLarge     : Number    = 413
        /** HTTP status code */     static const ReqTooLong         : Number    = 414
        /** HTTP status code */     static const UnsupportedType    : Number    = 415
        /** HTTP status code */     static const ServerError        : Number    = 500
        /** HTTP status code */     static const NotImplemented     : Number    = 501
        /** HTTP status code */     static const BadGateway         : Number    = 502
        /** HTTP status code */     static const Unavailable        : Number    = 503
        /** HTTP status code */     static const GatewayTimeout     : Number    = 504
        /** HTTP status code */     static const Version            : Number    = 505

        /**
         *  Create an Http object. The object is initialized with the URI, but no connection is done until one of the
         *  connection methods is called.
         *  @param uri The (optional) URI to initialize with.
         *  @throws IOError if the URI is malformed.
         */
        native function Http(uri: String = null)


        /**
         *  Add a request header. Must be done before the Http request is issued. 
         *  @param key The header keyword for the request, e.g. "accept".
         *  @param value The value to associate with the header, e.g. "yes"
         *  @param overwrite If true, overwrite existing headers of the same key name.
         */
        native function addRequestHeader(key: String, value: String, overwrite: Boolean = true): Void


        /**
         *  Get the number of data bytes that are currently available on this stream for reading.
         *  @returns The number of available bytes.
         */
        native function get available(): Number 


        /**
         *  Define a request callback and put the Http object into async mode. The specified function will be invoked as 
         *  response data is received on request completion and on any errors. After the callback has been called, the
         *  After response data will be discarded. The callback function may call $available to see what data is ready for
         *  reading or may simply call $read to retrieve available data. In this mode, calls to $connect, $get, $post, $del,
         *  $head, $options or $trace  will not block. 
         *  @param cb Callback function to invoke when the HTTP request is complete and some response data is
         *      available to read. The callback is invoked with the signature: 
         *  <pre>
         *      function callback(e: Event): Void
         *  </pre>
         *  Where e.data == http. The even arg may be either a HttpDataEvent Issued when there is response data to read or 
         *  on end of request ($available == 0). The HttpError event will be passed  on any request processing errors. Does 
         *  not include remote server errors.
         */
        native function set callback(cb: Function): Void


        //  TODO - bug require getter
        /** @hide */
        native function get callback(): Function


        /**
         *  Close the connection to the server and free up all associated resources.
         *  @param graceful if true, then close the socket gracefully after writing all pending data.
         */
        native function close(graceful: Boolean = true): Void 


        /**
         *  Issue a HTTP request for the current method and uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately. The HTTP method should be defined via the $method property and uri via the $uri
         *  property.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function connect(): Void


        /**
         *  Get the name of the file holding the certificate for this HTTP object. This method applies only if secure
         *      communications are in use and if setCertificationFile has been called. 
         *  @return The file name.
         */
        native function get certificateFile(): String


        /**
         *  Set or reset the file name where this Https object holds its certificate. If defined and secure 
         *  communications are used, then the client connection will supply this certification during its 
         *  connection request.
         *  @param certFile The name of the certificate file.
         *  @throws IOError if the file does not exist.
         */
        native function set certificateFile(certFile: String): Void


        /**
         *  Get the response code from a Http message, e.g. 200.
         *  @return The integer value of the response code or -1 if not known.
         */
        native function get code(): Number


        /**
         *  Get the response code message from a Http message, e.g. OK.
         *  @return The string message returned with the HTTP response status line.
         */
        native function get codeString(): String


        /**
         *  Get the value of the content encoding.
         *  @return A string with the content type or null if not known.
         */
        native function get contentEncoding(): String


        /**
         *  Get the response content length.
         *  @return The integer value or -1 if not known.
         */
        native function get contentLength(): Number


        /**
         *  Get the response content type. Call content() to get the resonse content.
         *  @return A string with the content type or null if not known.
         */
        native function get contentType(): String


        /**
         *  Get the value of the content date header field.
         *  @return The date the request was served.
         */
        native function get date(): Date


        /**
         *  Issue a DELETE request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function del(uri: String = null): Void


        /**
         *  Get the value of the expires header field.
         *  @return The date the content expires
         */
        native function get expires(): Date


        /**
         *  Flush any buffered data
         */
        function flush(): Void {
        }

        /**
         *  Get whether redirects should be automatically followed by this Http object.
         *  @return True if redirects are automatically followed.
         */
        native function get followRedirects(): Boolean


        /**
         *  Eanble or disable following redirects from the connection remove server. Default is true.
         *  @param flag Set to true to follow redirects.
         */
        native function set followRedirects(flag: Boolean): Void


        /**
         *  Issue a POST request with form data the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri Optional request uri. If non-null, this overrides any previously defined uri for the Http object.
         *  @param postData Optional object hash of key value pairs to use as the post data. These are www-url-encoded and
         *      the content mime type is set to "application/x-www-form-urlencoded".
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function form(uri: String, postData: Object): Void


        /**
         *  Issue a GET request for the current uri. This function will block until the the request completes or fails, unless 
         *  a callback function has been defined. If a callback has been specified, this function will initiate the request and 
         *  return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function get(uri: String = null): Void


        /**
         *  Issue a HEAD request for the current uri. This function will block until the the request completes or fails, unless 
         *  a callback function has been defined. If a callback has been specified, this function will initiate the request and 
         *  return immediately.
         *  @param uri The request uri. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function head(uri: String = null): Void


        /**
         *  Get the value of any response header field.
         *  @return The header field value as a string or null if not known.
         */
        native function header(key: String): String


        /**
         *  Get all the response headers
         *  @return An object hash filled with all the headers.
         */
        native function get headers(): Object


        /**
         *  Get whether the connection is utilizing SSL.
         *  @return True if the connection is using SSL.
         */
        native function get isSecure(): Boolean


        /**
         *  Get the filename of the private key for this Https object.
         *  @return The file name.
         */
        native function get keyFile(): String


        /**
         *  Set or reset the filename where this Https object holds its private key.
         *  @param keyFile The name of the key file.
         *  @throws IOError if the file does not exist.
         */
        native function set keyFile(keyFile: String): Void


        /**
         *  Get the value of the response's last modified header field.
         *  @return The number of milliseconds since January 1, 1970 GMT or -1 if not known.
         */
        native function get lastModified(): Date


        /**
         *  Get the request method for this Http object.
         *  @return The request string or null if not set.
         */
        native function get method(): String


        /**
         *  Set or reset the Http object's request method. Default method is GET.
         *  @param name The method name as a string.
         *  @throws IOError if the request is not GET, POST, HEAD, OPTIONS, PUT, DELETE or TRACE.
         */
        native function set method(name: String)


        /**
         *  Issue a POST request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately. NOTE: the posted data is NOT URL encoded. If you want to post data to a form,
         *  consider using the $form method.
         *  @param uri Optional request uri. If non-null, this overrides any previously defined uri for the Http object.
         *  @param data Optional data to send with the post request. 
         *  @param data Data objects to send with the post request. Data is written raw and is not encoded or converted. 
         *      However, post intelligently handles arrays such that, each element of the array will be written. If posting
         *      to a form, consider the $form method. 
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function post(uri: String, ...data): Void


        /**
         *  Set the post request length. This sets the Content-Length header value and is used when using $write to 
         *  emit the post data.
         *  @param length The length of post data that will be written via writePostData
         */
        native function set postLength(length: Number): Void

        //  TODO BUG - requires setter
        native function get postLength(): Number

                                                       
        /**
         *  Set the post data to send with a post request.
         *  @param items An object hash of key value pairs to use as the post data
         */
        native function set postData(items: Object): Void


        /**
         *  Issue a PUT request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @param data Data objects to write to the request stream. Data is written raw and is not encoded or converted. 
         *      However, put intelligently handles arrays such that, each element of the array will be written. If encoding 
         *      of put data is required, use the BinaryStream filter. 
         *  @param putData Optional object hash of key value pairs to use as the post data.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function put(uri: String, ...putData): Void


        /**
         *  Read a block of response content data from the connection. This will automatically call $connect if required.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data. If offset is not supplied, data is appended at the 
         *      current byte array write position.
         *  @param count Number of bytes to read. 
         *  @returns a count of the bytes actually read. Returns zero if no data is available. Returns -1 when the end of 
         *      response data has been reached.
         *  @throws IOError if an I/O error occurs.
         */
        native function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number


        /**
         *  Read the request response as a string. This will automatically call $connect if required.
         *  @param count of bytes to read. Returns the entire response contents if count is -1.
         *  @returns a string of $count characters beginning at the start of the response data.
         *  @throws IOError if an I/O error occurs.
         */
        native function readString(count: Number = -1): String


        /**
         *  Read the request response as an array of lines. This will automatically call $connect if required.
         *  @param count of linese to read. Returns the entire response contents if count is -1.
         *  @returns a string containing count lines of data starting with the first line of response data
         *  @throws IOError if an I/O error occurs.
         */
        native function readLines(count: Number = -1): Array


        /**
         *  Read the request response as an XML document. This will automatically call $connect if required.
         *  @returns the response content as an XML object 
         *  @throws IOError if an I/O error occurs.
         */
        native function readXml(): XML


        /**
         *  Return a stream connected to the request post/put data stream. This will automatically call $connect if required.
         *  @return A stream to write the request post or put data.
         */
        native function get requestStream(): Stream


        /**
         *  Return the response data. This is an alias for $readString(). This will automatically call $connect if required.
         *  @returns the response as a string of characters.
         *  @throws IOError if an I/O error occurs.
         */
        function get response(): String {
            return readString()
        }


        /**
         *  Return a stream connected to the response data. This will automatically call $connect if required.
         *  @return A stream to read the response data.
         */
        native function get responseStream(): Stream


        /**
         *  Set the user credentials to use if the request requires authentication.
         */
        native function setCredentials(username: String, password: String): Void


        /**
         *  Get the request timeout. 
         *  @returns Number of milliseconds for requests to block while attempting the request.
         */
        native function get timeout(): Number


        /**
         *  Set the request timeout.
         *  @param timeout Number of milliseconds to block while attempting requests. -1 means no timeout.
         */
        native function set timeout(timeout: Number): Void


        /**
         *  Issue a HTTP get request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri Request uri. If not null, this will override any previously defined uri for the Http object. 
         *  @param filename File to upload.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function upload(uri: String, filename: String): Void


        /**
         *  Get the URI for this Http object.
         *  @return The current uri string.
         */
        native function get uri(): String


        /**
         *  Set or reset the Http object's URI. If the Http object is already connected the connection is first closed.
         *  @param uri The URI as a string.
         *  @throws IOError if the URI is malformed.
         */
        native function set uri(uriString: String): Void


        /**
         *  Write post data to the request stream. The http object must be in async mode by previously defining a $callback.
         *  The write call blocks while writing data. The Http.postLength should normally be set prior to writing any data to 
         *  define the post data Content-length header value. If it is not defined, the content-length header will not be 
         *  defined and the remote server will have to close the underling HTTP connection at the completion of the request. 
         *  This will prevent HTTP keep-alive for subsequent requests. This call will automatically call $connect if required.
         *  @param data Data objects to write to the request stream. Data is written raw and is not encoded or converted. 
         *      However, write intelligently handles arrays such that, each element of the array will be written. If encoding 
         *      of write data is required, use the BinaryStream filter. 
         *  @throws StateError if the Http method is not set to POST.
         *  @throws IOError if more post data is written than specified via the setPostDataLength function.
         */
        native function write(...data): Void
    }


    /**
     *  Data event issued to the callback function.
     */
    class HttpDataEvent extends Event {
    }


    /**
     *  Error event issued to the callback function if any errors occur during an Http request.
     */
    class HttpErrorEvent extends Event {
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/Http.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/TextStream.es"
 */
/************************************************************************/

/*
 *  TextStream.es -- TextStream class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  TODO - encoding
 */

module ejs.io {

    /**
     *  TextStreams interpret data as a stream of Unicode characters. They provide methods to read and write data
     *  in various text encodings and to read/write lines of text appending appropriate system dependent new line terminators.
     *  TextStreams can be stacked upon other Streams such as files, byte arrays, sockets, or Http objects.
     *  @spec ejs-11
     */
    class TextStream implements Stream {

        use default namespace public

        /** Text encoding formats for use with $encoding */
        static const LATIN1: String = "latin1"

        /** Text encoding formats for use with $encoding */
        static const UTF_8: String = "utf-8"

        /** Text encoding formats for use with $encoding */
        static const UTF_16: String = "utf-16"

        /**
         *  System dependent newline terminator
         */
        private var newline: String = "\n"

        /*
         *  Data input and output buffers
         */
        private var inbuf: ByteArray

        //  TODO - this should come from the default encoding somewhere in Locale
        private var format: String = UTF_8

        /*
         *  Provider Stream
         */
        private var nextStream: Stream


        /**
         *  Create a text filter stream. A Text filter stream must be stacked upon a stream source such as a File.
         *  @param stream stream data source/sink to stack upon.
         */
        function TextStream(stream: Stream) {
            if (stream == null) {
                throw new ArgError("Must supply a Stream argument")
            }

            inbuf = new ByteArray(System.Bufsize, true)
            inbuf.input = fill
            nextStream = stream

            if (Config.OS == "WIN") {
                newline = "\r\n"
            }
        }


        /**
         *  Close the input stream and free up all associated resources.
         */
        function close(graceful: Boolean = true): Void {
            inbuf.flush()
            nextStream.close(graceful)
        }


        /**
         *  Get the current text encoding.
         *  @returns the current text encoding as a string.
         */
        function get encoding(): String {
            return format
        }


        /**
         *  Set the current text encoding.
         *  @param encoding string containing the current text encoding. Supported encodings are: utf-8.
         */
        function set encoding(encoding: String = UTF_8): Void {
            format = encoding
        }


        /*
         *  Fill the input buffer from upstream
         *  @returns The number of new characters added to the input bufer
         */
        private function fill(): Number {
            let was = inbuf.available
            inbuf.reset()
            count = nextStream.read(inbuf)
            return inbuf.available - was
        }


        /**
         *  Flush the stream and the underlying file data. Will block while flushing. Note: may complete before
         *  the data is actually written to disk.
         */
        function flush(): Void {
            inbuf.flush()
            nextStream.flush()
        }


        /**
         *  Read characters from the stream into the supplied byte array. This routine is used by upper streams to read
         *  data from the text stream as raw bytes.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data. If < 0, then read to the write position.
         *  @param count Number of bytes to read. 
         *  @returns a count of characters actually written
         *  @throws IOError if an I/O error occurs.
         */
        function read(buffer: ByteArray, offset: Number = -1, count: Number = -1): Number {
            let total = 0
            if (count < 0) {
                count = Number.MaxValue
            }
            let where = offset
            if (offset < 0) {
                where = buffer.writePosition
                buffer.reset()
            }
            while (count > 0) {
                if (inbuf.available == 0 && fill() == 0) {
                    break
                }
                let len = inbuf.available.min(count)
                buffer.copyIn(where, inbuf, inbuf.readPosition, len)
                where += len
                inbuf.readPosition += len
                total += len
                count -= len
            }
            if (offset < 0) {
                buffer.writePosition += total
            }
            return total
        }


        /**
         *  Read a line from the stream.
         *  @returns A string containing the next line without the newline character
         *  @throws IOError if an I/O error occurs.
         */
        function readLine(): String {
            let start = inbuf.readPosition
            if (inbuf.available == 0 && fill() == 0) {
                return null
            }
            while (true) {
                let c = newline.charCodeAt(0)
                for (let i = start; i < inbuf.writePosition; i++) {
                    if (inbuf[i] == c) {
                        if (newline.length == 2 && (i+1) < inbuf.writePosition && newline.charCodeAt(1) != inbuf[i+1]) {
                            continue
                        }
                        result = inbuf.readString(i - inbuf.readPosition)
						inbuf.readPosition += newline.length
                        return result
                    }
                }
                start = inbuf.writePosition
                if (fill() == 0) {
                    /*
                     *  Missing a line terminator, so return the last portion of text
                     */
                    result = inbuf.readString()
                    return result
                }
            }
            return null
        }


        /**
         *  Read a required number of lines of data from the stream.
         *  @param numLines of lines to read. Defaults to read all lines.
         *  @returns Array containing the read lines.
         *  @throws IOError if an I/O error occurs.
         */
        function readLines(numLines: Number = -1): Array {
            var result: Array
            if (numLines <= 0) {
                result = new Array
                numLines = Number.MaxValue
            } else {
                result = new Array(numLines)
            }
            for (let i in numLines) {
                if ((line = readLine()) == null) 
                    break
                result[i] = line
            }
            return result
        }


        /**
         *  Read a string from the stream. 
         *  @param count of bytes to read. Returns the entire stream contents if count is -1.
         *  @returns a string
         *  @throws IOError if an I/O error occurs.
         */
        function readString(count: Number = -1): String {
            return inbuf.readString(count)
        }


        /**
         *  Write characters to the stream. The characters will be encoded using the current TextStream encoding.
         *  endpoint can accept more data.
         *  @param data String to write. 
         *  @returns The total number of elements that were written.
         *  @throws IOError if there is an I/O error.
         */
        function write(...data): Number {
            return nextStream.write(data)
        }


        /**
         *  Write text lines to the stream. The text line is written after appending the system text newline character or
         *  characters. 
         *  @param lines Text lines to write.
         *  @return The number of characters written or -1 if unsuccessful.
         *  @throws IOError if the file could not be written.
         */
        function writeLine(...lines): Number {
            let written = 0
            for each (let line in lines) {
                var count = line.length
                written += nextStream.write(line)
                written += nextStream.write(newline)
            }
            return written
        }
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/TextStream.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/Url.es"
 */
/************************************************************************/

/*
 *  Url.es -- URL parsing and management class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

//  TODO - who is using this???
//  TODO - add encode / decode routines

    /**
     *  URL class to manage URLs
     *  @spec ejs-11
     */
    # FUTURE
    class Url {

        use default namespace public

        native var protocol: String;

        native var host: String;

        native var port: Number;

        native var path: String;

        native var extension: String;

        native var query: String;


        /**
         *  Create and parse a URL object. 
         *      be set later.
         *  @param url A url string. The URL may optionally contain a protocol specification and host specification.
         *  @throws IOError if the URL is malformed.
         */
        native function Url(url: String)
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/Url.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/io/XMLHttp.es"
 */
/************************************************************************/

/**
 *  XMLHttp.es -- XMLHttp class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    /**
     *  XMLHttp compatible method to retrieve HTTP data
     *  @spec ejs-11
     */
    class XMLHttp {

        use default namespace public

        private var hp: Http = new Http
        private var state: Number = 0
        private var response: ByteArray

        //  TODO spec UNSENT
        /** readyState values */
        static const Uninitialized = 0              

        //  TODO spec OPENED
        /** readyState values */
        static const Open = 1

        //  TODO spec HEADERS_RECEIVED
        /** readyState values */
        static const Sent = 2

        //  TODO spec LOADING
        /** readyState values */
        static const Receiving = 3

        //  TODO spec DONE
        /** readyState values */
        static const Loaded = 4

        /**
         *  Call back function for when the HTTP state changes.
         */
        public var onreadystatechange: Function


        /**
         *  Abort the connection
         */
        function abort(): void {
            hp.close
        }


        /**
         *  Return the underlying Http object
         *  @returns The Http object providing the implementation for XMLHttp
         *  @spec ejs-11
         */
        function get http() : Http {
            return hp
        }


        /**
         *  Return the readystate value. This value can be compared with the XMLHttp constants: Uninitialized, Open, Sent,
         *  Receiving or Loaded
         *  @returns Returns a number with the value: Uninitialized = 0, Open = 1, Sent = 2, Receiving = 3, Loaded = 4
         */
        function get readyState() : Number {
            return state
        }


        /**
         *  Return the HTTP response payload as text.
         *  @returns Returns a string containing the HTTP request response data.
         */
        function get responseText(): String {
            return response.toString()
        }


        /**
         *  Return the HTTP response payload as an XML document
         *  @returns Returns an XML object that is the root of the HTTP request response data.
         */
        function get responseXML(): XML {
            return XML(response.toString())
        }


        /**
         *  Not implemented. Only for ActiveX on IE
         */
        function get responseBody(): String {
            throw new Error("Unsupported API")
            return ""
        }


        /**
         *  Get the HTTP status code.
         *  @returns a status code between 100 and 600. See $Http for the status code constants.
         */
        function get status(): Number {
            return hp.code
        }


        /**
         *  Return the HTTP status code message
         *  @returns a string containing the status message returned by the web server
         */
        function get statusText() : String {
            return hp.codeString
        }


        /**
         *  Return the response headers
         *  @returns a string with the headers catenated together.
         */
        function getAllResponseHeaders(): String {
            let result: String = ""
            for (key in hp.headers) {
                result = result.concat(key + ": " + hp.headers[key] + '\n')
            }
            return result
        }


        /**
         *  Return a response header. Not yet implemented.
         *  @param key The name of the response key to be returned.
         *  @returns the header value as a string
         */
        function getResponseHeader(key: String) {
            //  TODO - not implemented
        }


        /**
         *  Open a connection to the web server using the supplied URL and method.
         *  @param method HTTP method to use. Valid methods include "GET", "POST", "PUT", "DELETE", "OPTIONS" and "TRACE"
         *  @param url URL to invoke
         *  @param async If true, don't block after issuing the requeset. By defining an $onreadystatuschange callback function,
         *      the request progress can be monitored.
         *  @param user Optional user name if authentication is required.
         *  @param password Optional password if authentication is required.
         */
        function open(method: String, url: String, async: Boolean = false, user: String = null, password: String = null): Void {
            hp.method = method
            hp.url = url
            if (userName && password) {
                hp.setCredentials(user, password)
            }
            hp.callback = callback
            response = new ByteArray(System.Bufsize, 1)

            hp.connect()
            state = Open
            notify()

            if (!async) {
                let timeout = 5 * 1000
                let when: Date = new Date
                while (state != Loaded && when.elapsed < timeout) {
                    App.serviceEvents(1, timeout)
                }
            }
        }


        /**
         *  Send data with the request.
         *  @param content Data to send with the request.
         */
        function send(content: String): Void {
            if (hp.callback == null) {
                throw new IOError("Can't call send in sync mode")
            }
            hp.write(content)
        }


        /**
         *  Set an HTTP header with the request
         *  @param key Key value for the header
         *  @param value Value of the header
         *  @example:
         *      setRequestHeader("Keep-Alive", "none")
         */
        function setRequestHeader(key: String, value: String): Void {
            //  TODO - is overwrite correct?
            hp.addRequestHeader(key, value, 1)
        }

        /*
         *  Http callback function
         */
        private function callback (e: Event) {
            if (e is HttpError) {
                notify()
                return
            }
            let hp: Http = e.data
            let count = hp.read(response)
            state = (count == 0) ? Loaded : Receiving
            notify()
        }

        /*
         *  Invoke the user's state change handler
         */
        private function notify() {
            if (onreadystatechange) {
                onreadystatechange()
            }
        }
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/io/XMLHttp.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/App.es"
 */
/************************************************************************/

/*
 *	App.es -- Application configuration and control. (Really controlling the interpreter's environment)
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

    //  TODO - confusion with the Application class
	/**
	 *	Application configuration class. This is a singleton class which exposes methods to interrogate and control
     *	the applications environment.
	 *	@spec ejs-11
	 */
	native class App {

        use default namespace public

		//	TODO - what about other supported encodings? These should be PascalCase
		static const UTF_8: Number = 1
		static const UTF_16: Number = 2

		/**
		 *	Application command line arguments.
         *	@returns an array containing each of the arguments. If the ejs command is invoked as "ejs script arg1 arg2", then
         *	args[0] will be "script", args[1] will be "arg1" etc.
		 */
		native static function get args(): Array


		//	TODO - change name to exeDir. Confusion with workingDir
		/**
		 *	Return the directory containing the application executable
		 *	@return a string containing the directory path for the application executable
		 */
		native static function get dir(): String


		/**
		 *	Get an environment variable.
		 *	@param name The name of the environment variable to retrieve.
		 *	@return The value of the environment variable or null if not found.
		 */
		native static function getEnv(name: String): String
		

		/**
		 *	Set the standard error stream.
		 *	@param stream The output stream.
		 */
        # FUTURE
		native static function set errorStream(stream: Stream): void
		

		/**
		 *	Gracefully stop the program and exit the interpreter.
		 *	@param status The optional exit code to provide the environment.
		 */
		native static function exit(status: Number = 0): void


		/**
		 *	Set the standard input stream.
		 *	@param stream The input stream.
		 */
        # FUTURE
		native static function set inputStream(stream: Stream): void
		

		//	TODO - need to be able to test what modules are loaded and get a list of loaded modules. Perhaps a Module class.
		/**
		 *	Load an Ejscript module into the interpreter.
		 *	@param The name of the module.
		 *	@return False if could not find the module or it was already loaded or some other error, true otherwise.
		 */
        # FUTURE
		native static function loadModule(path: String): Boolean


		/**	
		 *	Get the current language locale for this application
		 */
        # FUTURE
		native static function get locale(): String


		//	TODO - move to a Locale class
		/**	
		 *	Set the current language locale
		 */
        # FUTURE
		native static function set locale(locale: String): void


		/**
		 *	Application name. 
         *	@returns a single word, lower case name for the application.
		 */
		static function get name(): String {
			return Config.Product
		}


		//	TODO need a better name than noexit, TODO could add a max delay option.
		/**
		 *	Control whether an application will exit when global scripts have completed. Setting this to true will cause
		 *	the application to continue servicing events until the $exit method is explicitly called. The default application 
		 *	setting of noexit is false.
		 *	@param exit If true, the application will exit when the last script completes.
		 */
		native static function noexit(exit: Boolean = true): void


		/**
		 *	Get the default permissions to use when creating files
		 */
        # FUTURE
		native static function get permissions(): Number


		/**
		 *	Set the default permissions to use when creating files
		 *	@param value New umask to use for creating new files
		 */
        # FUTURE
		native static function set permissions(value: Number = 0664): void


		/**
		 *	Set the standard output stream.
		 *	@param stream The output stream.
		 */
        # FUTURE
		native static function set outputStream(stream: Stream): void
		

		/**
		 *	Service events
         *	@param count Count of events to service. Defaults to unlimited.
         *	@param timeout Timeout to block waiting for an event in milliseconds before returning. If an event occurs, the
         *	    call returns immediately.
		 */
        native static function serviceEvents(count: Number = -1, timeout: Number = -1): Void


		/**
		 *	Set an environment variable.
		 *	@param env The name of the environment variable to set.
		 *	@param value The new value.
		 *	@return True if the environment variable was successfully set.
		 */
        # FUTURE
		native static function setEnv(name: String, value: String): Boolean


		/**
		 *	Sleep the application for the given number of milliseconds
		 *	@param delay Time in milliseconds to sleep. Set to -1 to sleep forever.
		 */
		native static function sleep(delay: Number = -1): Void


		/**
		 *	Application title name. Multi word, Camel Case name for the application.
         *	@returns the name of the application suitable for printing.
		 */
		static static function get title(): String {
			return Config.Title
		}


		/**
		 *	Application version string. 
         *	@returns a version string of the format Major.Minor.Patch. For example: 1.1.2
		 */
		static static function get version(): String {
			return Config.Version
		}


		/**
		 *	Get the application's Working directory
		 *	@return the path to the working directory
		 */
		native static function get workingDir(): String


		/**
		 *	Set the application's Working directory
		 *	@param The path to the new working directory
		 */
		native static function set workingDir(value: String): Void
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/App.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/Config.es"
 */
/************************************************************************/

/*
 *	Config.es - Configuration settings from ./configure
 *
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 */

module ejs.sys {

	use default namespace public

	/* NOTE: These values are updated at run-time by src/types/sys/ejsConfig.c */

	/**
	 *	Config class providing settings for various ./configure settings.
	 */
	native class Config extends Object {

        use default namespace public

		/**
		 *	True if a debug build
		 */
		static const Debug: Boolean

		/**
		 *	CPU type (eg. i386, ppc, arm)
		 */
		static const CPU: String

		/**
		 *	Build with database (SQLite) support
		 */
		static const DB: Boolean

		/**
		 *	Build with E4X support
		 */
		static const E4X: Boolean

		/**
		 *	Build with floating point support
		 */
		static const Floating: Boolean

		/**
		 *	Build with HTTP client support 
		 */
		static const Http: Boolean

		/**
		 *	Language specification level. (ecma|plus|fixed)
		 */
	    static const Lang: String

		/**
		 *	Build with legacy API support
		 */
		static const Legacy: Boolean

		/**
		 *	Build with multithreading support
		 */
		static const Multithread: Boolean

		/**
		 *	Number type
		 */
		static const NumberType: String

		/**
		 *	Operating system version. One of: WIN, LINUX, MACOSX, FREEBSD, SOLARIS
		 */
		static const OS: String

		/**
		 *	Ejscript product name. Single word name.
		 */
		static const Product: String

		/**
		 *	Regular expression support.
		 */
		static const RegularExpressions: Boolean

		/**
		 *	Ejscript product title. Multiword title.
		 */
		static const Title: String

		/**
		 *	Ejscript version. Multiword title. Format is Major.Minor.Patch-Build For example: 1.1.2-1
		 */
		static const Version: String

        /**
         *  Installation library directory
         */
        static const LibDir: String

        /**
         *  Binaries directory
         */
        static const BinDir: String
	}
}

/************************************************************************/
/*
 *  End of file "../es/sys/Config.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/Debug.es"
 */
/************************************************************************/

/*
 *	Debug.es -- Debug class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Debug configuration class. Singleton class containing the application's debug configuration.
	 *	@spec ejs-11
	 */
    # FUTURE
	class Debug {

        use default namespace public

		/**
		 *	Break to the debugger. Suspend execution and break to the debugger.
		 */ 
		native function breakpoint(): void


		/**
		 *	Get the debug mode. This property is read-write. Setting @mode to true will put the application in debug mode. 
		 *	When debug mode is enabled, the runtime will typically suspend timeouts and will take other actions to make 
		 *	debugging easier.
		 *	@see breakpoint
		 */
		native function get mode(): Boolean


		/**
		 *	Set the application debug mode. When debugging is enabled, timeouts are suspended, virtual machine logging 
		 *	verbosity is increased and other actions taken to make debugging easier.
		 *	@param value True to turn debug mode on or off.
		 */
		native function set mode(value: Boolean): void
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/Debug.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/GC.es"
 */
/************************************************************************/

/*
 *	GC.es -- Garbage collector class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Garbage collector control class. Singleton class to control operation of the Ejscript garbage collector.
	 *	@spec ejs-11
	 */
	native class GC {

        use default namespace public

        /*
         *  Get the current heap usage.
         *  @return the amount of heap memory used by ejscript and the portable runtime in bytes
         */
        native static function get allocatedMemory(): Number


		/**
		 *	Test if the garbage collector is enabled. 
         *	@return True if enabled
		 *	the garbage collector. The default value is true.
		 */
		native static function get enabled(): Boolean


		/**
		 *	Enable or disable the garbage collector
         *	@param on Set to true to enable the collector.
		 */
		native static function set enabled(on: Boolean): Void


        /*
         *  Get the maximum amount of heap memory the application can consume. This is the amount of memory used by
         *  ejscript and the portable runtime. It will be less than the total application memory. If the application 
         *  exceeds this amount a MemoryError exception will be thrown and the application will go into a graceful degrate
         *  mode. In this mode, memory will be aggressively conserved and the application should immediately do an 
         *  orderly exit.
         *  @return The maximum permissable amount of heap memory (bytes) that can be consumed by ejscript
         */
		native static function get maxMemory(): Number


        /*
         *  Set the maximum amount of heap memory ejscript and the portable runtime can consume. This is not equal to total 
         *  application memory.
         *  @param limit The maximum permissable amount of heap memory (bytes) that can be consumed by ejscript.
         */
		native static function set maxMemory(limit: Number): Void


        /*
         *  Print memory consumption stats to the console (stdout)
         */
        native static function printStats(): Void


        /*
         *  Get the peak heap usage.
         *  @return the peak amount of heap memory used by ejscript and the portable runtime in bytes
         */
        native static function get peakMemory(): Number


		/**
		 *	Get the quota of work to perform before the GC will be invoked. 
         *	@return The number of work units that will trigger the GC to run. This roughly corresponds to the number
         *	of allocated objects.
		 */
		native static function get workQuota(): Number


		/**
		 *	Set the quota of work to perform before the GC will be invoked. 
         *	@param quota The number of work units that will trigger the GC to run. This roughly corresponds to the number
         *	of allocated objects.
		 */
		native static function set workQuota(quota: Number): Void


		/**
		 *	Run the garbage collector and reclaim memory allocated to objects and properties that are no longer reachable. 
		 *	When objects and properties are freed, any registered destructors will be called. The run function will run 
		 *	the garbage collector even if the @enable property is set to false. 
		 *	@param deep If set to true, will collect from all generations. The default is to collect only the youngest
		 *		geneartion of objects.
		 */
		native static function run(deep: Boolean = flase): void

	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/GC.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/Logger.es"
 */
/************************************************************************/

/*
 *	Logger.es - Log file control class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Logger objects provide a convenient and consistent method to capture and store logging information from 
	 *	applications. The verbosity and scope of the logging can be changed at start-up time. Logs can be sent
	 *	to various output and storage facilities (console, disk files, etc.) 
	 *
	 *	A logger may have a "parent" logger in order to create hierarchies of loggers for better logging control 
	 *	and granularity. For example, a logger can be created for each class in a package with all such loggers 
	 *	having a single parent. Loggers can send log messages to their parent and inherit their parent's log level. 
	 *	This allows for easier control of verbosity and scope of logging.  
 	 *
	 *	A logger may have a "filter", an arbitrary function, that returns true or false depending on whether a 
	 *	specific message should be logged or not. 
	 *	@spec ejs-11
	 */
    # FUTURE
	namespace BIG_SPACE

    # FUTURE
	class Logger {

        use default namespace public

		/**
		 *	Logging level for inherit level from parent.
		 */
		static const Inherit: Number = -1


		/**
		 *	Logging level for no logging.
		 */
		static const Off: Number = 0


		/**
		 *	Logging level for most serious errors.
		 */
		static const Error: Number = 1


		/**
		 *	Logging level for warnings.
		 */
		static const Warn: Number = 2


		/**
		 *	Logging level for informational messages.
		 */
		static const Info: Number = 3


		/**
		 *	Logging level for configuration output.
		 */
		static const Config: Number = 4


		/**
		 *	Logging level to output all messages.
		 */
		static const All: Number = 5


		/**
		 *	Do not output messages to any device.
		 */
		static const None: Number = 0


		/**
		 *	Output messages to the console.
		 */
		static const Console: Number = 0x1


		/**
		 *	Output messages to a file.
		 */
		static const LogFile: Number = 0x2


		/**
		 *	Output messages to an O/S event log.
		 */
		static const EventLog: Number = 0x4


		/**
		 *	Output messages to a in-memory store.
		 */
		static const MemLog: Number = 0x8


		/**
 		 * 	Logger constructor.
		 *	The Logger constructor can create different types of loggers based on the three (optional) arguments. 
		 *	The logging level can be set in the constructor and also changed at run-time. Where the logger output 
		 *	goes (e.g. console or file) is statically set. A logger may have a parent logger to provide hierarchical 
		 *	mapping of loggers to the code structure.
		 *	@param name Loggers are typically named after the namespace package or class they are associated with.
		 *	@param level Optional enumerated integer specifying the verbosity.
		 *	@param output Optional output device(s) to send messages to.
		 *	@param parent Optional parent logger.
		 *	@example:
		 *		var log = new Logger("name", 5, LogFile)
		 *		log(2, "message")
		 */
		native function Logger(name: String, level: Number = 0, output: Number = LogFile, parent: Logger = null)


		/**
		 *	Get the filter function for a logger.
		 *	@return The filter function.
		 */
		native function get filter(): Function


		/**
		 *	Set the filter function for this logger. The filter function is called with the following signature:
		 *
		 *		function filter(log: Logger, level: Number, msg: String): Boolean
		 *
		 *	@param function The filter function must return true or false.
		 */
		native function set filter(filter: Function): void


		/**
		 *	Get the verbosity setting (level) of this logger.
		 *	@return The level.
		 */
		native function get level(): Number


		/**
		 *	Set the output level of this logger. (And all child loggers who have their logging level set to Inherit.)
		 *	@param level The next logging level (verbosity).
		 */
		native function set level(level: Number): void


		/**
		 *	Get the name of this logger.
		 *	@return The string name.
		 */
		native function get name(): String


		/**
		 *	Set the name for this logger.
		 *	@param name An optional string name.
		 */
		native function set name(name: String): void


		/**
		 *	Get the devices this logger sends messages to.
		 *	@return The different devices OR'd together.
		 */
		native function get output(): Number


		/**
		 *	Set the output devices for this logger.
		 *	@param name Logically OR'd list of devices.
		 */
		native function set output(ouput: Number): void


		/**
		 *	Get the parent of this logger.
		 *	@return The parent logger.
		 */
		native function get parent(): Logger


		/**
		 *	Set the parent logger for this logger.
		 *	@param parent A logger.
		 */
		native function set parent(parent: Logger): void


		/**
		 *	Record a message via a logger. The message level will be compared to the logger setting to determine 
		 *	whether it will be output to the devices or not. Also, if the logger has a filter function set that 
		 *	may filter the message out before logging.
		 *	@param level The level of the message.
		 *	@param msg The string message to log.
		 */
		native function log(level: Number, msg: String): void


		/**
		 *	Convenience method to record a configuration change via a logger.
		 *	@param msg The string message to log.
		 */
		native function config(msg: String): void


		/**
		 *	Convenience method to record an error via a logger.
		 *	@param msg The string message to log.
		 */
		native function error(msg: String): void


		/**
		 *	Convenience method to record an informational message via a logger.
		 *	@param msg The string message to log.
		 */
		native function info(msg: String): void


		/**
		 *	Convenience method to record a warning via a logger.
		 *	@param msg The string message to log.
		 */
		native function warn(msg: String): void
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/Logger.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/Memory.es"
 */
/************************************************************************/

/*
 *	Memory.es -- Memory statistics
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Singleton class to monitor and report on memory allocation by the application for a given interpreter.
	 *	@spec ejs-11
	 */
	native class Memory {

        use default namespace public

		/**
		 *	Memory redline. When the memory redline limit is exceeded, a $MemoryError exception will be thrown and 
		 *	the interpreter will go into graceful degrade mode. Subsequent memory allocations up to the $maxMemory 
		 *	limit will succeed allowing a graceful recovery or exit of the application. 
		 */
        #FUTURE
		native static function get redline(): Number
        #FUTURE
		native static function set redline(value: Number): Void


		/**
		 *	Maximum memory that may be used. This defines the upper limit for memory usage by the application. If 
		 *	this limit is reached, subsequent memory allocations will fail and a $MemoryError exception will be 
		 *	thrown. Setting it to zero will allow unlimited memory allocations up to the system imposed maximum.
		 *	If $redline is defined and non-zero, $MemoryError exception will be thrown when the $redline is exceeded.
		 */
        #FUTURE
		native static function get maxMemory(): Number
        #FUTURE
		native static function set maxMemory(value: Number): Void


		/**
		 *	Available memory for the application. This is the maximum amount of memory the application may ever 
		 *	request from the operating system. This is the maximum number that $consumedMemrory will ever return.
		 */
        #FUTURE
		native static function get availableMemory(): Number


		/**
		 *	Total memory consumed by the application. This includes memory currently in use and also memory that 
		 *	has been freed but is still retained by the garbage collector for future use.
		 */
        #FUTURE
		native static function get consumedMemory(): Number


		/**
		 *	Peak memory ever used by the application. This statistic is the maximum value ever attained by $usedMemory. 
		 */
        #FUTURE
		native static function get peakMemory(): Number
		

		/**
		 *	Peak stack size ever used by the application. 
		 */
        #FUTURE
		native static function get peakStack(): Number
		

		/**
		 *	System RAM. This is the total amount of RAM installed in the system.
		 */
        #FUTURE
		native static function get systemRam(): Number
		

		/**
		 *	Total memory used to host the application. This includes all memory used by the application and 
		 *	the interpreter. It is measured by the O/S.
		 */
        #FUTURE
		native static function get totalMemory(): Number
		

		/**
		 *	Memory currently in-use by the application for objects. This does not include memory allocated but not 
		 *	in-use (see $consumedMemory). It thus represents the current memory requirements.
		 */
        #FUTURE
		native static function get usedMemory(): Number


		/**
		 *	Prints memory statistics to the debug log
		 */
		native static function printStats(): void
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/Memory.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/System.es"
 */
/************************************************************************/

/*
 *	System.es - System class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	System is a utility class providing methods to interact with the operating system.
	 *	@spec ejs-11
	 */
	native class System {

        use default namespace public

        public static const Bufsize: Number = 1024

		/**
		 *	Get the system hostname
		 *	@param fullyQualified If true, return a hostname including a domain portion
		 *	@return A host name string
		 */
		native static function get hostname(fullyQualified: Boolean = true): String


		/**
		 *	Execute a command/program using the default operating system shell.
		 *	@param command Command or program to execute
		 *	@return a text stream connected to the programs standard output.
         *  @throws IOError if the command exits with non-zero status. 
		 */
		native static function run(cmd: String): String

		native static function runx(cmd: String): Void

        # FUTURE
		native static function cmd(cmd: String): Stream
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/System.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/sys/Unix.es"
 */
/************************************************************************/

/*
 *	Unix.es -- Unix compatibility functions
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

    use default namespace public

	/**
	 *	Get the base name of a file. Returns the base name portion of a file name. The base name portion is the 
	 *	trailing portion without any directory elements.
	 *	@return A string containing the base name portion of the file name.
	 */
	function basename(path: String): String {
        return new File(path).basename
    }
	

    /**
     *	Close the file and free up all associated resources.
     *	@param file Open file object previously opened via $open or $File
     *	@param graceful if true, then close the file gracefully after writing all pending data.
     */
    function close(file: File, graceful: Boolean = true): Void {
        file.close(graceful)
    }


	/**
	 *	Copy a file. If the destination file already exists, the old copy will be overwritten as part of the copy operation.
	 *	@param fromPath Original file to copy.
	 *	@param toPath New destination file path name.
	 *	@throws IOError if the copy is not successful.
	 */
	function cp(fromPath: String, toPath: String): void {
        new File(fromPath).copy(toPath) 
    }
	

    /**
     *	Get the directory name portion of a file. The dirname name portion is the leading portion including all 
     *	directory elements and excluding the base name. On some systems, it will include a drive specifier.
     *	@return A string containing the directory name portion of the file name.
     */
    function dirname(path: String): String {
        return new File(path).dirname
    }


	/**
	 *	Does a file exist. Return true if the specified file exists and can be accessed.
	 *	@param path Filename path to examine.
	 *	@return True if the file can be accessed
	 */
	function exists(path: String): Boolean {
        return new File(path).exists
    }


    /**
     *	Get the file extension portion of the file name.
     *  @param path Filename path to examine
     *	@return String containing the file extension
     */
    function extension(path: String): String  {
        return new File(path).extension
    }


	/**
	 *	Return the free space in the file system.
	 *	@return The number of 1M blocks (1024 * 1024 bytes) of free space in the file system.
	 */
	native function freeSpace(path: String = null): Number
	

	/**
	 *	Is a file a directory. Return true if the specified path exists and is a directory
	 *	@param path Directory path to examine.
	 *	@return True if the file can be accessed
	 */
	function isDir(path: String): Boolean {
        return new File(path).isDir
    }


	//	TODO - good to add ability to do a regexp on the path or a filter function
	//	TODO - good to add ** to go recursively to any depth
	/**
	 *	Get a list of files in a directory. The returned array contains the base file name portion only.
	 *	@param path Directory path to enumerate.
	 *	@param enumDirs If set to true, then dirList will include sub-directories in the returned list of files.
	 *	@return An Array of strings containing the filenames in the directory.
	 */
	function ls(path: String, enumDirs: Boolean = false): Array {
        return new File(path).getFiles(enumDirs)
    }


	/**
	 *	Make a new directory. Makes a new directory and all required intervening directories. If the directory 
	 *	already exists, the function returns without throwing an exception.
	 *	@param path Filename path to use.
	 *	@throws IOError if the directory cannot be created.
	 */
	function mkdir(path: String, permissions: Number = 0755): void {
        new File(path).makeDir(permissions)
    }
	

	/**
	 *	Rename a file. If the new file name exists it is removed before the rename.
	 *	@param from Original file name.
	 *	@param to New file name.
	 *	@throws IOError if the original file does not exist or cannot be renamed.
	 */
	function mv(fromFile: String, toFile: String): void {
        new File(fromFile).rename(toFile)
    }
	

    /**
     *  Open or create a file
     *  @param path Filename path to open
     *	@param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. Defaults to Read
     *	@param permissions optional permissions. Defaults to App.permissions
     *	@return a File object which implements the Stream interface
     *	@throws IOError if the path or file cannot be opened or created.
     */
    function open(path: String, mode: Number = Read, permissions: Number = 0644): File {
        let file: File = new File(path)
        file.open(mode, permissions)
        return file
    }


    /**
     *  Get the current working directory
     */
	function pwd(): String {
        return App.workingDir
    }


    /**
     *	Read data bytes from a file and return a byte array containing the data.
     *	@param file Open file object previously opened via $open or $File
     *	@return A byte array containing the read data
     *	@throws IOError if the file could not be read.
     */
    function read(file: File, count: Number): ByteArray {
        return file.read(count)
    }


	/**
	 *	Remove a file from the file system.
	 *	@param path Filename path to delete.
	 *	@param quiet Don't complain if the file does not exist.
	 *	@throws IOError if the file exists and cannot be removed.
	 */
	function rm(path: String): void {
        new File(path).remove()
    }


	//	TODO - recursive remove?
	/**
	 *	Removes a directory. 
	 *	@param path Filename path to remove.
	 *	@throws IOError if the directory exists and cannot be removed.
	 */
	function rmdir(path: String, recursive: Boolean = false): void {
        new File(path).removeDir(recursive)
    }


	/**
	 *	Create a temporary file. Creates a new, uniquely named temporary file.
	 *	@param directory Directory in which to create the temp file.
	 *	@returns a closed File object after creating an empty temporary file.
	 */
	function tempname(directory: String = null): File {
        return File.createTempFile(directory)
    }


    /**
     *	Write data to the file. If the stream is in sync mode, the write call blocks until the underlying stream or 
     *	endpoint absorbes all the data. If in async-mode, the call accepts whatever data can be accepted immediately 
     *	and returns a count of the elements that have been written.
     *	@param file Open file object previously opened via $open or $File
     *	@param items The data argument can be ByteArrays, strings or Numbers. All other types will call serialize
     *	first before writing. Note that numbers will not be written in a cross platform manner. If that is required, use
     *	the BinaryStream class to write Numbers.
     *	@returns the number of bytes written.  
     *	@throws IOError if the file could not be written.
     */
    function write(file: File, ...items): Number {
        return file.write(items)
    }

}

/*
 *  @copy	default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/sys/Unix.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/connectors/GoogleConnector.es"
 */
/************************************************************************/

/**
 *	GoogleConnector.es -- View connector for the Google Visualization library
 */

module ejs.web {

	class GoogleConnector {

        use default namespace "ejs.web"

        private var nextId: Number = 0

        private function scriptHeader(kind: String, id: String): Void {
            write('<script type="text/javascript" src="http://www.google.com/jsapi"></script>')
            write('<script type="text/javascript">')
            write('  google.load("visualization", "1", {packages:["' + kind + '"]});')
            write('  google.setOnLoadCallback(' + 'draw_' + id + ');')
        }


        //  TODO - must support Ejs options: bands, columns, data, onClick, refresh, pageSize, pivot, 
        //      sortColumn, sortOrder, style, styleHead, styleEvenRow, styleOddRow, styleCell, visible, widths
        //      Support @options
        /**
         *  @duplicate View.table
         */
		function table(grid: Array, options: Object): Void {
            var id: String = "GoogleTable_" + nextId++

			if (grid == null || grid.length == 0) {
				write("<p>No Data</p>")
				return
			}
            let columns: Array = options["columns"]

            scriptHeader("table", id)
            
            write('  function ' + 'draw_' + id + '() {')
			write('    var data = new google.visualization.DataTable();')

            let firstLine: Object = grid[0]
            if (columns) {
                if (columns[0] != "id") {
                    columns.insert(0, "id")
                }
                for (let i = 0; i < columns.length; ) {
                    if (firstLine[columns[i]]) {
                        i++
                    } else {
                        columns.remove(i, i)
                    }
                }
            } else {
                columns = []
                for (let name in firstLine) {
                    columns.append(name)
                }
            }

            for each (name in columns) {
                write('    data.addColumn("string", "' + name.toPascal() + '");')
			}
			write('    data.addRows(' + grid.length + ');')

			for (let row: Object in grid) {
                let col: Number = 0
                for each (name in columns) {
                    write('    data.setValue(' + row + ', ' + col + ', "' + grid[row][name] + '");')
                    col++
                }
            }

            write('    var table = new google.visualization.Table(document.getElementById("' + id + '"));')

            let goptions = getOptions(options, { 
                height: null, 
                page: null,
                pageSize: null,
                showRowNumber: null,
                sort: null,
                title: null,
                width: null, 
            })

            write('    table.draw(data, ' + serialize(goptions) + ');')

            if (options.click) {
                write('    google.visualization.events.addListener(table, "select", function() {')
                write('        var row = table.getSelection()[0].row;')
                write('        window.location = "' + view.makeUrl(options.click, "", options) + '?id=" + ' + 
                    'data.getValue(row, 0);')
                write('    });')
            }

            write('  }')
            write('</script>')

            write('<div id="' + id + '"></div>')
		}


        //  TODO - must support Ejs options: columns, kind, onClick, refresh, style, visible
        //  TODO - use @options
        /**
         *  @duplicate View.chart
         */
		function chart(grid: Array, options: Object): Void {
            var id: String = "GoogleChart_" + nextId++

			if (grid == null || grid.length == 0) {
				write("<p>No Data</p>")
				return
			}

            let columns: Array = options["columns"]

            scriptHeader("piechart", id)
            
            write('  function ' + 'draw_' + id + '() {')
			write('    var data = new google.visualization.DataTable();')

			let firstLine: Object = grid[0]
            let col: Number = 0
            //  TODO - need to get data types
            let dataType: String = "string"
			for (let name: String in firstLine) {
                if  (columns && columns.contains(name)) {
                    write('    data.addColumn("' + dataType + '", "' + name.toPascal() + '");')
                    col++
                    if (col >= 2) {
                        break
                    }
                    dataType = "number"
                }
			}
			write('    data.addRows(' + grid.length + ');')

			for (let row: Object in grid) {
                let col2: Number = 0
                //  TODO - workaround
				for (let name2: String in grid[row]) {
                    if  (columns && columns.contains(name2)) {
                        if (col2 == 0) {
                            write('    data.setValue(' + row + ', ' + col2 + ', "' + grid[row][name2] + '");')
                        } else if (col2 == 1) {
                            write('    data.setValue(' + row + ', ' + col2 + ', ' + grid[row][name2] + ');')
                        }
                        //  else break. TODO should do this but it is bugged
                        col2++
                    }
                }
            }

            //  PieChart, Table
            write('    var chart = new google.visualization.PieChart(document.getElementById("' + id + '"));')

            let goptions = getOptions(options, { width: 400, height: 400, is3D: true, title: null })
            write('    chart.draw(data, ' + serialize(goptions) + ');')

            write('  }')
            write('</script>')

            write('<div id="' + id + '"></div>')
		}


        /**
         *  Parse an option string
         */
        private function getOptions(options: Object, defaults: Object): Object {
            var result: Object = {}
            for (let word: String in defaults) {
                if (options[word]) {
                    result[word] = options[word]
                } else if (defaults[word]) {
                    result[word] = defaults[word]
                }
            }
            return result
        }


        private function write(str: String): Void {
            view.write(str)
        }
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/web/connectors/GoogleConnector.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/connectors/HtmlConnector.es"
 */
/************************************************************************/

/**
 *	HtmlConnector.es -- Basic HTML control connector
 */

module ejs.web {

    use module ejs.db

	/**
	 *	The Html Connector provides bare HTML encoding of Ejscript controls
     *	TODO - should actually implement the ViewConnector
	 */
	class HtmlConnector {

        use default namespace "ejs.web"

        /*
         *  Options to implement:
         *      method
         *      update
         *      confirm     JS confirm code
         *      condition   JS expression. True to continue
         *      success
         *      failure
         *      query
         *
         *  Not implemented
         *      submit      FakeFormDiv
         *      complete
         *      before
         *      after
         *      loading
         *      loaded
         *      interactive
         */
        /**
         *  @duplicate View.aform
         */
		function aform(record: Object, url: String, options: Object): Void {
            if (options.id == undefined) {
                options.id = "form"
            }
            onsubmit = ""
            if (options.condition) {
                onsubmit += options.condition + ' && '
            }
            if (options.confirm) {
                onsubmit += 'confirm("' + options.confirm + '"); && '
            }
            onsubmit = '$.ajax({ ' +
                'url: "' + url + '", ' + 
                'type: "' + options.method + '", '

            if (options.query) {
                onsubmit += 'data: ' + options.query + ', '
            } else {
                onsubmit += 'data: $("#' + options.id + '").serialize(), '
            }

            if (options.update) {
                if (options.success) {
                    onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); ' + 
                        options.success + '; }, '
                } else {
                    onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); }, '
                }
            } else if (options.success) {
                onsubmit += 'success: function(data) { ' + options.success + '; } '
            }
            if (options.error) {
                onsubmit += 'error: function(data) { ' + options.error + '; }, '
            }
            onsubmit += '}); return false;'

            write('<form action="' + "/User/list" + '"' + getOptions(options) + "onsubmit='" + onsubmit + "' >")
        }


        /*
         *  Extra options:
         *      method
         *      update
         *      confirm     JS confirm code
         *      condition   JS expression. True to continue
         *      success
         *      failure
         *      query
         *
         */
        /**
         *  @duplicate View.alink
         */
		function alink(text: String, url: String, options: Object): Void {
            if (options.id == undefined) {
                options.id = "alink"
            }
            onclick = ""
            if (options.condition) {
                onclick += options.condition + ' && '
            }
            if (options.confirm) {
                onclick += 'confirm("' + options.confirm + '"); && '
            }
            onclick = '$.ajax({ ' +
                'url: "' + url + '", ' + 
                'type: "' + options.method + '", '

            if (options.query) {
                'data: ' + options.query + ', '
            }

            if (options.update) {
                if (options.success) {
                    onclick += 'success: function(data) { $("#' + options.update + '").html(data); ' + 
                        options.success + '; }, '
                } else {
                    onclick += 'success: function(data) { $("#' + options.update + '").html(data); }, '
                }
            } else if (options.success) {
                onclick += 'success: function(data) { ' + options.success + '; } '
            }
            if (options.error) {
                onclick += 'error: function(data) { ' + options.error + '; }, '
            }
            onclick += '}); return false;'

            write('<a href="' + options.url + '"' + getOptions(options) + "onclick='" + onclick + "' >" + text + '</a>')
		}


        /**
         *  @duplicate View.button
         */
		function button(value: String, buttonName: String, options: Object): Void {
            write('<input name="' + buttonName + '" type="submit" value="' + value + '"' + getOptions(options) + ' />')
        }


        /**
         *  @duplicate View.buttonLink
         */
		function buttonLink(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"><button>' + text + '</button></a>')
        }


        /**
         *  @duplicate View.chart
         */
		function chart(data: Array, options: Object): Void {
            //  TODO
            throw 'HtmlConnector control "chart" not implemented.'
		}


        /**
         *  @duplicate View.checkbox
         */
		function checkbox(name: String, value: String, submitValue: String, options: Object): Void {
            let checked = (value == submitValue) ? ' checked="yes" ' : ''
            write('<input name="' + name + '" type="checkbox" "' + getOptions(options) + checked + 
                '" value="' + submitValue + '" />')
        }


        /**
         *  @duplicate View.endForm
         */
		function endform(): Void {
            write('</form>')
        }


        /**
         *  TODO  - how to make this style-able?
         */
		function flash(kind: String, msg: String, options: Object): Void {
            write('<div' + getOptions(options) + '>' + msg + '</div>')
            if (kind == "inform") {
                //  TODO - should be based on ID
                write('<script>$(document).ready(function() {
                        $("div.flashInform").animate({opacity: 1.0}, 2000).hide("slow");});
                    </script>')
            }
		}


        /**
         *  @duplicate View.form
         */
		function form(record: Object, url: String, options: Object): Void {
            write('<form action="' + url + '"' + getOptions(options) + '>')
//          write('<input name="id" type="hidden" value="' + record.id + '" />')
        }


        /**
         *  @duplicate View.image
         */
        function image(src: String, options: Object): Void {
			write('<img src="' + src + '"' + getOptions(options) + '/>')
        }


        /**
         *  @duplicate View.label
         */
        function label(text: String, options: Object): Void {
            write('<span ' + getOptions(options) + ' type="' + getTextKind(options) + '">' +  text + '</span>')
        }


        /**
         *  @duplicate View.link
         */
		function link(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"' + getOptions(options) + '>' + text + '</a>')
		}


        /**
         *  @duplicate View.extlink
         */
		function extlink(text: String, url: String, options: Object): Void {
			write('<a href="' + url + '"' + getOptions(options) + '>' + text + '</a>')
		}


        /**
         *  @duplicate View.list
         */
		function list(name: String, choices: Object, defaultValue: String, options: Object): Void {
            write('<select name="' + name + '" ' + getOptions(options) + '>')
            let isSelected: Boolean
            let i = 0
            for each (choice in choices) {
                if (choice is Array) {
                    isSelected = (choice[0] == defaultValue) ? 'selected="yes"' : ''
                    write('  <option value="' + choice[0] + '"' + isSelected + '>' + choice[1] + '</option>')
                } else {
                    if (choice && choice.id) {
                        for (field in choice) {
                            isSelected = (choice.id == defaultValue) ? 'selected="yes"' : ''
                            if (field != "id") {
                                write('  <option value="' + choice.id + '"' + isSelected + '>' + choice[field] + '</option>')
                            }
                        }
                    } else {
                        isSelected = (i == defaultValue) ? 'selected="yes"' : ''
                        write('  <option value="' + i + '"' + isSelected + '>' + choice + '</option>')
                    }
                }
                i++
            }
            write('</select>')
        }


        /**
         *  @duplicate View.mail
         */
		function mail(name: String, address: String, options: Object): Void  {
			write('<a href="mailto:' + address + '" ' + getOptions(options) + '>' + name + '</a>')
		}


        /**
         *  @duplicate View.progress
         */
		function progress(data: Array, options: Object): Void {
            write('<p>' + data + '%</p>')
		}


        //  Emit: <input name ="model.name" id="id" class="class" type="radio" value="text"
        /**
         *  @duplicate View.radio
         */
        function radio(name: String, selected: String, choices: Object, options: Object): Void {
            let checked: String
            if (choices is Array) {
                for each (v in choices) {
                    checked = (v == selected) ? "checked" : ""
                    write(v + ' <input type="radio" name="' + name + '"' + getOptions(options) + 
                        ' value="' + v + '" ' + checked + ' />\r\n')
                }
            } else {
                for (item in choices) {
                    checked = (choices[item] == selected) ? "checked" : ""
                    write(item + ' <input type="radio" name="' + name + '"' + getOptions(options) + 
                        ' value="' + choices[item] + '" ' + checked + ' />\r\n')
                }
            }
        }


		/** 
		 *	@duplicate View.script
		 */
		function script(url: String, options: Object): Void {
            write('<script src="' + url + '" type="text/javascript"></script>\r\n')
		}


        /**
         *  @duplicate View.status
         */
		function status(data: Array, options: Object): Void {
            write('<p>' + data + '</p>\r\n')
        }


		/** 
		 *	@duplicate View.stylesheet
		 */
		function stylesheet(url: String, options: Object): Void {
            write('<link rel="stylesheet" type="text/css" href="' + url + '" />\r\n')
		}


        /**
         *  @duplicate View.tabs
         */
		function tabs(tabs: Array, options: Object): Void {
            write('<div class="menu">')
            write('<ul>')
            for each (t in tabs) {
                for (name in t) {
                    let url = t[name]
                    write('<li><a href="' + url + '">' + name + '</a></li>\r\n')
                }
            }
            write('</ul>')
            write('</div>')
        }


        /**
         *  @duplicate View.table
         */
		function table(data: Array, options: Object = {}): Void {
            //  TODO - should take a model
			if (data == null || data.length == 0) {
				write("<p>No Data</p>")
				return
			}

            if (options.title) {
                write('    <h2 class="ejs tableHead">' + options.title + '</h2>')
            }

/*
            if (options.filter) {
                //  TODO - should we be using options and getOptions here?
                url = view.makeUrl(controller.actionName, "", options)
                write('<form action="' + url + '"' + getOptions(options) + '>')
                write('<input type="text" name="ejs::table" id="" class="ejs tableHead">')
                write('</form>')
            }
*/
			write('<table ' + getOptions(options) + '>')

			write('  <thead class="ejs">')
			write('  <tr>')

            let line: Object = data[0]
            let columns: Object = options["columns"]

            if (columns) {
                for (name in columns) {
                    let column = columns[name]
                    if (line[name] == undefined && column.render == undefined) {
                        throw new Error("Can't find column \"" + name + "\" in data set: " + serialize(line))
                        columns[name] = null
                    }
                }

                /*
                 *  Emit column headings
                 */
                for (let name in columns) {
                    if (name == null) {
                        continue
                    }
                    let header: String

                    //  TODO compiler BUG header = (columns[name].header) ? (columns[name].header) : name.toPascal()
                    if (columns[name].header == undefined) {
                        header = name.toPascal()
                    } else {
                        header = columns[name].header
                    }
                    let width = (columns[name].width) ? ' width="' + columns[name].width + '"' : ''
                    write('    <th class="ejs"' + width + '>' + header + '</th>')
                }

            } else {
                columns = {}
                for (let name in line) {
                    if (name == "id" && !options.showId) {
                        continue
                    }
                    write('    <th class="ejs">' + name.toPascal() + '</th>')
                    columns[name] = {}
                }
            }

			write("  </tr>\r\n</thead>")

            let row: Number = 0
			for each (let r: Object in data) {
				write('  <tr class="ejs">')
                let url: String = null
                if (options.click) {
                    url = view.makeUrl(options.click, r.id, options)
                }
                //  TODO - should implement styleOddRow, styleEvenRow on rows not on cells
                //  TODO - should implement styleCell
                let style = (row % 2) ? "oddRow" : "evenRow"
				for (name in columns) {
                    if (name == null) {
                        continue
                    }

                    let column = columns[name]
                    let cellStyle: String

                    if (column.style) {
                        cellStyle = style + " " + column.style
                    } else {
                        cellStyle = style
                    }
                    //  TODO OPT
                    data = view.getValue(r, name, { render: column.render } )
                    if (url) {
//  TODO - what other styles should this be?
//  TODO - better to use onclick
                        write('    <td class="ejs ' + cellStyle + '"><a href="' + url + '">' + data + '</a></td>')
                    } else {
                        write('    <td class="ejs ' + cellStyle + '">' + data + '</td>')
                    }
				}
                row++
				write('  </tr>')
			}
			write('</table>')
		}


        //  Emit: <input name ="model.name" id="id" class="class" type="text|hidden|password" value="text"
        /**
         *  @duplicate View.text
         */
        function text(name: String, value: String, options: Object): Void {
            write('<input name="' + name + '" ' + getOptions(options) + ' type="' + getTextKind(options) + 
                '" value="' + value + '" />')
        }


        // Emit: <textarea name ="model.name" id="id" class="class" type="text|hidden|password" value="text"
        /**
         *  @duplicate View.textarea
         */
        function textarea(name: String, value: String, options: Object): Void {
            numCols = options.numCols
            if (numCols == undefined) {
                numCols = 60
            }
            numRows = options.numRows
            if (numRows == undefined) {
                numRows = 10
            }
            write('<textarea name="' + name + '" type="' + getTextKind(options) + '" ' + getOptions(options) + 
                ' cols="' + numCols + '" rows="' + numRows + '">' + value + '</textarea>')
        }


        /**
         *  @duplicate View.tree
         */
        function tree(data: Array, options: Object): Void {
            throw 'HtmlConnector control "tree" not implemented.'
        }


        private function getTextKind(options): String {
            var kind: String

            if (options.password) {
                kind = "password"
            } else if (options.hidden) {
                kind = "hidden"
            } else {
                kind = "text"
            }
            return kind
        }


		private function getOptions(options: Object): String {
            return view.getOptions(options)
        }


        private function write(str: String): Void {
            view.write(str)
        }
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/web/connectors/HtmlConnector.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Controller.es"
 */
/************************************************************************/

/**
 *  Controller.es -- Ejscript Controller class as part of the MVC framework. Also contains control helpers for views.
 */

module ejs.web {

    use module ejs.db

    namespace action = "action"                 /* Namespace for all action methods */

    //  TODO - call currentView
    /**
     *  Current view in the web framework
     */
    var view: View

    /**
     *  Web framework controller. Part of the Ejscript web MVC framework.
     */
    class Controller {

        /*
         *  Define properties and functions (by default) in the ejs.web namespace so that user controller variables 
         *  don't clash.
         */
        use default namespace "ejs.web"


        //  TODO - why are these public?
        /**
         *  Name of the action being run
         */
        public var actionName:  String 
        public var originalActionName:  String 

        /**
         *  Stores application global data. The application array provides a means to store persistent information 
         *  to be shared across all clients using an application. Objects and variables stored in the application 
         *  array will live until either explicitly deleted or the web server exits. The application array does not 
         *  persist over system reboots. The elements are User defined.
         *  NOTE: Not yet implemented.
         */
        public /* synchronized */ var application: Object

        /**
         *  Base URL for the application. Does not include the URL scheme or host name portion.
         */
        public var appUrl:      String

        /**
         *   Lower case controller name 
         */
        public var controllerName: String

        /**
         *  Web application configuration. This is initialized from the config *.ecf files.
         */
        public var config:      Object

        //  TODO - needs work - names are not same gramatical types
        /**
         *  Flash messages to display on the next screen
         *      "inform"        Informational / postitive feedback (note)
         *      "message"       Neutral feedback (reminders, suggestions...)
         *      "warning"       Negative feedback (Warnings and errors)
         *      "error"         Negative errors (Warnings and errors)
         */
        public var flash:       Object

        /**
         *  Physical home directory of the application
         */
        public var home:        String

        /**
         *  Host object
         */
        public var host:        Host

        /**
         *  Form parameters. 
         */
        public var params:      Object

        /**
         *  The request object stores details of the incoming client's request
         */
        public var request:     Request

        /**
         *  The response object stores details of the response going back to the client.
         */
        public var response:    Response

        /**
         *  Stores session state information. The session array will be created automatically if SessionAutoCreate 
         *  is defined or if a session is started via the useSession() or createSession() functions. Sessions are 
         *  shared among requests that come from a single client. This may mean that multiple requests access the 
         *  same session concurrently. Ejscript ensures that such accesses are serialized. The elements are user defined.
         */
        public /* synchronized */ var session: Object

        private var isApp:      Boolean
        private var rendered:   Boolean
        private var redirected: Boolean
        private var events:     Dispatcher

        private var _afterFilters: Array
        private var _beforeFilters: Array
        private var _wrapFilters: Array


        /**
         *  Controller initialization. This is specially hand-crafted by the hosting web server so that it runs
         *  before the sub-classing constructors.
         *  @param appDir Set to the top level directory containing the application
         *  @param appUrl URL that points to the application
         *  @param session Session state object
         *  @param host Host object
         *  @param request Request object
         *  @param response Response object
         */
        function initialize(isApp: Boolean, appDir: String, appUrl: String, session: Session, host: Host, request: Request, 
                response: Response): Void {

            this.isApp = isApp
            this.home = appDir
            this.appUrl = appUrl
            this.session = session
            this.host = host
            this.request = request
            this.response = response

            /*
             *  Load application configuration. 
             */
            if (isApp) {
                config = deserialize("{ " + File.getString(appDir + "/config/config.ecf") + " }")
                config.database = deserialize("{ " + File.getString(appDir + "/config/database.ecf") + " }")
                config.view = deserialize("{ " + File.getString(appDir + "/config/view.ecf") + " }")

                let adapter: String = config.database[config.app.mode].adapter
                let dbname: String = config.database[config.app.mode].database

                if (adapter != "" && dbname != "") {
                    Database.defaultDatabase = new Database(appDir + "/" + dbname)

                    //  TODO BUGGED - must push down into database. This is not doing per model.
                    if (config.database[config.app.mode].trace) {
                        Record.trace(true)
                        Database.trace(true)
                    }
                }
            }

            // events = new Dispatcher
            rendered = false
            redirected = false
            //  TODO - better to do these lazily
            params = new Object
            let name: String = Reflect(this).name
            controllerName = name.trim("Controller")
        }


        /** 
         *  Add a cache-control header to direct the browser to not cache the response.
         */
        native function cache(enable: Boolean = true): Void


        /**
         *  Enable session control. This enables session state management for this request and other requests 
         *  from the browser. If a session has not already been created, this call creates a session and sets 
         *  the @sessionID property in the request object. If a session already exists, this call has no effect. 
         *  A cookie containing a session ID is automatically created and sent to the client on the first response 
         *  after creating the session. If SessionAutoCreate is defined in the configuration file, then sessions 
         *  will automatically be created for every web request and your Ejscript web pages do not need to call 
         *  createSession. Multiple requests may be sent from a client's browser at the same time. Ejscript will 
         *  ensure that accesses to the sesssion object are correctly serialized. 
         *  @param timeout Optional timeout for the session in seconds. If ommitted the default timeout is used.
         */
        native function createSession(timeout: Number): Void


        /**
         *  Destroy a session. This call destroys the session state store that is being used for the current client. If no 
         *  session exists, this call has no effect.
         */
        native function destroySession(): Void


        /**
         *  Discard all prior output
         */
        native function discardOutput(): Void


        function beforeFilter(fn, options: Object = null): Void {
            if (_beforeFilters == null) {
                _beforeFilters = []
            }
            _beforeFilters.append([fn, options])
        }

        function afterFilter(fn, options: Object = null): Void {
            if (_afterFilters == null) {
                _afterFilters = []
            }
            _afterFilters.append([fn, options])
        }

        function wrapFilter(fn, options: Object = null): Void {
            if (_wrapFilters == null) {
                _wrapFilters = []
            }
            _wrapFilters.append([fn, options])
        }

        private function runFilters(filters: Array): Void {
            if (!filters) {
                return
            }
            for each (filter in filters) {
                let fn = filter[0]
                let options = filter[1]
                if (options) {
                    only = options.only
                    if (only) {
                        if (only is String && actionName != only) {
                            continue
                        }
                        if (only is Array && !only.contains(actionName)) {
                            continue
                        }
                    } 
                    except = options.except
                    if (except) {
                        if (except is String && actionName == except) {
                            continue
                        }
                        if (except is Array && except.contains(actionName)) {
                            continue
                        }
                    }
                }
                fn.call(this)
            }
        }

        /**
         *  Invoke the named action. Internal use only. Called from ejsWeb.c.
         *  @param act Action name to invoke
         */
        function doAction(act: String): Void {
            if (act == "") {
                act = "index"
            }
            actionName = act

            use namespace action
            if (this[actionName] == undefined) {
                originalActionName = actionName
                actionName = "missing"
            }

            flash = session["__flash__"]
            if (flash == "" || flash == undefined) {
                flash = {}
            } else {
                session["__flash__"] = undefined
                lastFlash = flash.clone()
            }

            runFilters(_beforeFilters)

            if (!redirected) {
                try {
                    this[actionName]()

                } catch (e) {
                    reportError(Http.ServerError, "Error in action: " + actionName, e)
                    //  TODO BUG - return not taken
                    rendered = true
                    return
                }

                if (!rendered) {
                    renderView()
                }

                runFilters(_afterFilters)

                //  TODO - temp
                if (Record.db) {
                    Record.db.close()
                }
            }
            
            if (lastFlash) {
                for (item in flash) {
                    for each (old in lastFlash) {
                        if (hashcode(flash[item]) == hashcode(old)) {
                            delete flash[item]
                        }
                    }
                }
            }
            if (flash && flash.length > 0) {
                session["__flash__"] = flash
            }
            // Memory.printStats()
        }


        /**
         *  Send an error response back to the client. This calls discard.
         *  @param msg Message to display
         */
        native function sendError(code: Number, msg: String): Void


        /**
         *  Transform a string to be safe for output into an HTML web page. It does this by changing the
         *  ">", "<" and '"' characters into their ampersand HTML equivalents.
         *  @param s input string
         *  @returns a transformed HTML escaped string
         */
        function escapeHtml(s: String): String {
            return s.replace(/&/g,'&amp;').replace(/\>/g,'&gt;').replace(/</g,'&lt;').replace(/"/g,'&quot;')
        }


		/** 
		 *	HTML encode the arguments
         *  @param args Variable arguments that will be converted to safe html
         *  @return A string containing the encoded arguments catenated together
		 */
		function html(...args): String {
            result = ""
			for (let s: String in args) {
				result += escapeHtml(s)
			}
            return resul
		}


        /**
         *  Send a positive notification to the user. This is just a convenience instead of setting flash["inform"]
         *  @param msg Message to display
         */
        function inform(msg: String): Void {
            flash["inform"] = msg
        }


        function error(msg: String): Void {
            flash["error"] = msg
        }


        /**
         *  Control whether the HTTP connection is kept alive after this request
         *  @parm on Set to true to enable keep alive.
         */
        native function keepAlive(on: Boolean): Void


        /**
         *  Load a view. If path is not supplied, use the default view for the current action.
         *  @param viewName Name of the view to load. It may be a fully qualified path name or a filename relative to the 
         *      views directory for the current action, but without the '.ejs' extension.
         */
        native function loadView(path: String = null): Void


        /**
         *  Make a URL suitable for invoking actions. This routine will construct a URL Based on a supplied action name, 
         *  model id and options that may contain an optional controller name. This is a convenience routine remove from 
         *  applications the burden of building URLs that correctly use action and controller names.
         *  @params action The action name to invoke in the URL. If the name starts with "/", it is assumed to be 
         *      a controller name and it is used by itself.
         *  @params id The model record ID to select via the URL. Defaults to null.
         *  @params options The options string
         *  @return A string URL.
         *  @options url An override url to use. All other args are ignored.
         *  @options controller The name of the controller to use in the URL.
         */
        function makeUrl(action: String, id: String = null, options: Object = null): String {
            let cname : String 
            if (options && options["url"]) {
                return options.url
            }
            if (action.startsWith("/")) {
                return action
            }
            if (options == null || options["controller"] == null) {
                cname = controllerName
            } else {
                cname = options["controller"]
            }
            let url: String = appUrl.trim("/")
            if (url != "") {
                url = "/" + url
            }
            url += "/" + cname + "/" + action
            if (id && id != "" && id != "undefined" && id != null) {
                url += "?id=" + id
            }
            return url
        }


        /**
         *  Redirect the client to a new URL. This call redirects the client's browser to a new location specified 
         *  by the @url.  Optionally, a redirection code may be provided. Normally this code is set to be the HTTP 
         *  code 302 which means a temporary redirect. A 301, permanent redirect code may be explicitly set.
         *  @param url Url to redirect the client to
         *  @param code Optional HTTP redirection code
         */
        native function redirectUrl(url: String, code: Number = 302): Void


        //  TODO - doc incomplete
        /**
         *  Redirect to the given action
         *  Options: id controller
         */
        function redirect(action: String, id: String = null, options: Object = null): Void {
            redirectUrl(makeUrl(action, id, options))
            // rendered = true
            redirected = true
        }


        /**
         *  Render the raw arguments back to the client. The args are converted to strings.
         */
        function render(...args): Void { 
            rendered = true
            write(args)
        }


        /**
         *  Render a file's contents. 
         */
        function renderFile(filename: String): Void { 
            rendered = true
            let file: File = new File(filename)
            try {
                file.open(File.Read)
                while (true) {
                    writeRaw(file.read(4096))
                }
                file.close()
            } catch (e: Error) {
                reportError(Http.ServerError, "Can't read file: " + filename, e)
            }
            write(File.getString(filename))
        }


        # FUTURE
        function renderPartial(): void {
        }


        /**
         *  Render raw data
         */
        function renderRaw(...args): Void {
            rendered = true
            writeRaw(args)
        }


        # FUTURE
        function renderXml(): Void {}

        # FUTURE
        function renderJSON(): Void {}


        /**
         *  Render a view template
         */
        function renderView(viewName: String = null): Void {

            if (rendered) {
                throw new Error("renderView invoked but render has already been called")
                return
            }
            rendered = true

            if (viewName == null) {
                viewName = actionName
            }
            
            //  TODO - compiler bug work around
            let gotError = false
            try {
                let name = Reflect(this).name
                let viewClass: String = name.trim("Controller") + "_" + viewName + "View"
                if (global[viewClass] == undefined) {
                    loadView(viewName)
                }
                view = new global[viewClass](this)

            } catch (e: Error) {
                if (e.code == undefined) {
                    e.code = Http.ServerError
                }
                if (extension(request.url) == ".ejs") {
                    reportError(e.code, "Can't load page: " + request.url, e)
                } else {
                    reportError(e.code, "Can't load view: " + viewName + ".ejs" + " for " + request.url, e)
                }
                //  TODO - bug this return is not returning
                gotError = true
                return
            }

            if (!gotError) {
                try {
                    for (let n: String in this) {
                        view[n] = this[n]
                    }
                    view.render()
                } catch (e: Error) {
                    reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs".', e)
                } catch (msg) {
                    reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs". ' + msg)
                }
            }
        }


        /**
         *  Report errors back to the client
         *  @param msg Message to send to the client
         *  @param e Optional Error exception object to include in the message
         */
        private function reportError(code: Number, msg: String, e: Object = null): Void {
            if (code <= 0) {
                code = Http.ServerError
            }
            if (e) {
                e = e.toString().replace(/.*Error Exception: /, "")
            }
            if (host.logErrors) {
                if (e) {
                    msg += "\r\n" + e
                }
            } else {
                msg = "<h1>Ejscript error for \"" + request.url + "\"</h1>\r\n<h2>" + msg + "</h2>\r\n"
                if (e) {
                    msg += "<pre>" + escapeHtml(e) + "</pre>\r\n"
                }
                msg += '<p>To prevent errors being displayed in the "browser, ' + 
                    'use <b>"EjsErrors log"</b> in the config file.</p>\r\n'
            }
            sendError(code, msg)
        }


        //  TODO - reconsider arg order
        /**
         *  Define a cookie header to include in the reponse
         */
        native function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void


        /**
         *  of the format "keyword: value". If a header has already been defined and \a allowMultiple is false, 
         *  the header will be overwritten. If \a allowMultiple is true, the new header will be appended to the 
         *  response headers and the existing header will also be output. NOTE: case does not matter in the header keyword.
         *  @param header Header string
         *  @param allowMultiple If false, overwrite existing headers with the same keyword. If true, all headers are output.
         */
        native function setHeader(key: String, value: String, allowMultiple: Boolean = false): Void


        /**
         *  Set the HTTP response status code
         *  @param code HTTP status code to define
         */
        native function setHttpCode(code: Number): Void


        /**
         *  Send a status message to any registered status message view controls
         */
        function statusMessage(...args) {
            //  TODO 
        }


        /**
         *  Set the response body mime type
         *  @param format Mime type for the response. For example "text/plain".
         */
        native function setMimeType(format: String): Void


        /**
         *  Transform an escaped string into its original contents. This reverses the transformation done by $escapeHtml.
         *  It does this by changing &quot, &gt, &lt back into ", < and >.
         *  @param s input string
         *  @returns a transformed string
         */
        function unescapeHtml(s: String): String {
            return replace(/&amp/g,'&;').replace(/&gt/g,'>').replace(/&lt/g,'<').replace(/&quot/g,'"')
        }


        /**
         *  Send a warning message back to the client for display in the flash area. This is just a convenience instead of
         *  setting flash["warn"]
         *  @param msg Message to display
         */
        function warn(msg: String): Void {
            flash["warning"] = msg
        }


        /**
         *  Write text to the client. This call writes the arguments back to the client's browser. The arguments 
         *  are converted to strings before writing back to the client. Text written using write, will be buffered 
         *  up to a configurable maximum. This allows text to be written prior to setting HTTP headers with setHeader.
         *  @param args Text or objects to write to the client
         */
        native function write(...args): Void


        /**
         *  Send text back to the client which must first be HTML escaped
         *  @param args Objects to emit
         */
        function writeHtml(...args): Void {
            write(html(args))
        }


        //ZZZ - comment
        //  TODO - omit the new line
        native function writeRaw(...args): Void


        /**
         *  Missing action method. This method will be called if the requested action routine does not exist.
         */
        action function missing(): Void {
            //  TODO - should this (or not) send a non 200 code
            render("<h1>Missing Action</h1>")
            //  TODO - could still try to serve the view
            render("<h3>Action: \"" + originalActionName + "\" could not be found for controller \"" + 
                controllerName + "\".</h3>")
        }
    }

    //  TODO - need to be able to suppress doc somehow
    class _SoloController extends Controller {
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/web/Controller.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Cookie.es"
 */
/************************************************************************/

/**
 *  Cookie.es -- Cookie class
 */

module ejs.web {

    /**
     *  Cookie class to store parsed cookie strings
     */
    class Cookie {

        /**
         *  Name of the cookie
         */
        var name: String


        /**
         *  Value of the cookie
         */
        var value: String


        /**
         *  Domain in which the cookie applies
         */
        var domain: String


        /**
         *  URI path in which the cookie applies
         */
        var path: String
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/web/Cookie.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Host.es"
 */
/************************************************************************/

/**
 *	Host.es - Host class for the Ejscript web framework
 */

module ejs.web {

	/**
	 *	Web server host information. The server array stores information that typically does not vary from request to request. 
	 *	For a given virtual server, these data items will be constant across all requests.
	 */
	final class Host {

        use default namespace public


		/**
		 *	Home directory for the web documents
		 */
		native var documentRoot: String


		/**
		 *	Fully qualified name of the server. Should be of the form (http://name[:port])
		 */
		native var name: String


		/**
		 *	Host protocol (http or https)
		 */
		native var protocol: String


		/**
		 *	Set if the host is a virtual host
		 */	
		native var isVirtualHost: Boolean


		/**
		 *	Set if the host is a named virtual host
		 */
		native var isNamedVirtualHost: Boolean


		/**
		 *	Server software description
		 */
		native var software: String


        /**
         *  Log errors to the application log
         */
        native var logErrors: Boolean
	}
}
/************************************************************************/
/*
 *  End of file "../es/web/Host.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Request.es"
 */
/************************************************************************/

/**
 *	Request.es - Request class for the Ejscript web framework
 */

module ejs.web {

    /**
     *  HTTP request information. The request objects stores parsed information for incoming HTTP requests.
     */
	final class Request {

        use default namespace public

		/**
		 *	Accept header
		 */
		native var accept: String


		/**
		 *	AcceptCharset header
		 */
		native var acceptCharset: String


		/**
		 *	AcceptEncoding header
		 */
		native var acceptEncoding: String


		/**
		 *	Authentication access control list
		 */
		native var authAcl: String


		/**
		 *	Authentication group
		 */
		native var authGroup: String


		/**
		 *	Authentication method if authorization is being used (basic or digest)
		 */
		native var authType: String


		/**
		 *	Authentication user name
		 */
		native var authUser: String


		/**
		 *	Connection header
		 */
		native var connection: String


		/**
		 *	Posted content length (header: Content-Length)
		 */
		native var contentLength: Number


		/**
		 *	Stores Client cookie state information. The cookies object will be created automatically if the Client supplied 
		 *	cookies with the current request. Cookies are used to specify the session state. If sessions are being used, 
		 *	a session cookie will be sent to and from the browser with each request. The elements are user defined.
         *	TODO - would be better if this were a hash of pre-parsed Cookie objects.
		 */
		native var cookies: Object

			
		/**
		 *	Extension portion of the URL after aliasing to a filename.
		 */
		native var extension: String


		/**
		 *	Files uploaded as part of the request. For each uploaded file, an object is created in files. The name of the 
		 *	object is given by the upload field name in the request page form. This is an array of UploadFile objects.
		 */
		native var files: Array


		/**
		 *	Store the request headers. The request array stores all the HTTP request headers that were supplied by 
         *	the client in the current request. 
		 */
		native var headers: Object


		/**
		 *	The host name header
		 */
		native var hostName: String


		/**
		 *	Request method: DELETE, GET, POST, PUT, OPTIONS, TRACE
		 */
		native var method: String


		/**
		 *	Content mime type (header: Content-Type)
		 */
		native var mimeType: String


		/**
		 *	The portion of the path after the script name if extra path processing is being used. See the ExtraPath 
         *	directive.
		 */
		native var pathInfo: String


		/**
		 *	The physical path corresponding to PATH_INFO.
		 */
		native var pathTranslated


		/**
		 *	Pragma header
		 */
		native var pragma: String


		/**
		 *	Decoded Query string (URL query string)
		 */
		native var query: String


		/**
		 *	Raw request URI before decoding.
		 */
		native var originalUri: String


		/**
		 *	Name of the referring URL
		 */
		native var referrer: String


		/**
		 *	The IP address of the Client issuing the request.
		 */
		native var remoteAddress: String


		/**
		 *	The host address of the Client issuing the request
		 */
		native var remoteHost: String


		/**
		 *	Current session ID. Index into the $sessions object
		 */
		native var sessionID: String


		/**
		 *	The decoded request URL portion after stripping the scheme, host, extra path, query and fragments
		 */
		native var url: String


		/**
		 *	Name of the Client browser software set in the HTTP_USER_AGENT header
		 */
		native var userAgent: String
	}
}
/************************************************************************/
/*
 *  End of file "../es/web/Request.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Response.es"
 */
/************************************************************************/

/**
 *	Response.es - Response object for the Ejscript web framework.
 */

module ejs.web {

    /**
     *  HTTP response class. The Http response object stores information about HTTP responses.
     */
	final class Response {

        use default namespace public

		/**
		 *	HTTP response code
		 */
		native var code: Number


		/**
		 *	Response content length. TODO -- Makes no sense as this 
		 */
        # FUTURE
		native var contentLength: Number


		/**
		 *	Cookies to send to the client. These are currently stored in headers[]
		 */
        # FUTURE
		native var cookies: Object


		/**
		 *	Unique response tag - not generated  yet
		 */
        # FUTURE
		native var etag: String


		/**
		 *	Filename for the Script name
		 */
		native var filename: String


		/**
		 *	Reponse headers
		 */
		native var headers: Array


		/**
		 *	Response content mime type
		 */
		native var mimeType: String
	}
}
/************************************************************************/
/*
 *  End of file "../es/web/Response.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/Session.es"
 */
/************************************************************************/

/**
 *  Session.es -- Session state management
 */

module ejs.web {

    /**
     *  Array of all sessions
     */
    var sessions = []

    /**
     *  Session state storage class
     */
    dynamic class Session {
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../es/web/Session.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/UploadFile.es"
 */
/************************************************************************/

/**
 *	uploadedFile.es - 
 */

module ejs.web {

    /**
     *  Upload file class. 
     */
	class UploadFile {

        use default namespace public

		/**
		 *	Name of the uploaded file given by the client
		 */
		native var clientFilename: String


		/**
		 *	Mime type of the encoded data
		 */
		native var contentType: String


		/**
		 *	Name of the uploaded file. This is a temporary file in the upload directory.
		 */
		native var filename: String


		var name: String

 
		/**
		 *	Size of the uploaded file in bytes
		 */
		native var size: Number
	}
}

/************************************************************************/
/*
 *  End of file "../es/web/UploadFile.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/web/View.es"
 */
/************************************************************************/

/**
 *	View.es -- View class as part of the Ejscript MVC framework
 */
module ejs.web {

    use module ejs.db

	/**
	 *	Base class for web framework views. This class provides the core functionality for all Ejscript view web pages.
     *	Ejscript web pages are compiled to create a new View class which extends the View base class. In addition to
     *	the properties defined by this class, user view classes will inherit at runtime all public properites of the
     *	current controller object.
     */

	dynamic class View {

        /*
         *  Define properties and functions are (by default) in the ejs.web namespace rather than public to avoid clashes.
         */
        use default namespace "ejs.web"

        /**
         *  Current controller
         *  TODO - consistency. Should this be currentController
         */
        var controller: Controller

        /*
         *  Current model being used inside a form
         *  TODO - what about nested forms?
         */
        private var currentModel: Record

        /*
         *  Configuration from the applications config *.ecf
         */
        private var config: Object

        /**
         *  Constructor method to initialize a new View
         *  @param controller Controller to manage this view
         */
        function View(controller: Controller) {
            this.controller = controller
            this.config = controller.config
        }


		/**
		 *	Process and emit a view to the client. Overridden by the views invoked by controllers.
		 */
		public function render(): Void {}

        /************************************************ View Helpers ****************************************************/

        //  TODO - how does this work if you don't want to associate a model record?
        //  TODO - should the type of the first arg be Record?
        //  TODO - should have an option to not show validation errors
        /**
         *  Render an asynchronous (ajax) form.
         *  @param action Action to invoke when the form is submitted. Defaults to "create" or "update" depending on 
         *      whether the field has been previously saved.
         *  @param record Model record to edit
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 *	@option url String Use a URL rather than action and controller for the target url.
         */
        function aform(action: String, record: Object = null, options: Object = null): Void {
            if (record == null) {
                record = new Model
            }
            currentModel = record
            formErrors(record)
            options = setOptions("aform", options)
            if (options.method == null) {
                options.method = "POST"
            }
            if (action == null) {
                action = "update"
            }
            let connector = getConnector("aform", options)
            options.url = makeUrl(action, record.id, options)
            connector.aform(record, options.url, options)
        }


		/** 
		 *	Emit an asynchronous (ajax) link to an action. The URL is constructed from the given action and the 
         *	    current controller. The controller may be overridden by setting the controller option.
         *  @param text Link text to display
         *  @param action Action to invoke when the link is clicked
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 *	@option controller String Name of the target controller for the given action
		 *	@option url String Use a URL rather than action and controller for the target url.
		 */
		function alink(text: String, action: String = null, options: Object = null): Void {
            if (action == null) {
                action = text.split(" ")[0].toLower()
            }
            options = setOptions("alink", options)
            if (options.method == null) {
                options.method = "POST"
            }
            let connector = getConnector("alink", options)
            options.url = makeUrl(action, options.id, options)
            connector.alink(text, options.url, options)
		}


        /**
         *  Render a form button. This creates a button suitable for use inside an input form. When the button is clicked,
         *  the input form will be submitted.
         *  @param value Text to display in the button.
         *  @param name Form name of button.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  Examples:
         *      button("Click Me", "OK")
         */
		function button(value: String, buttonName: String = null, options: Object = null): Void {
            options = setOptions("button", options)
            if (buttonName == null) {
                buttonName = value.toLower()
            }
            let connector = getConnector("button", options)
            connector.button(value, buttonName, options)
        }


        /**
         *  Render a link button. This creates a button suitable for use outside an input form. When the button is clicked,
         *  the associated URL will be invoked.
         *  @param text Text to display in the button.
         *  @param url Target URL to invoke when the button is clicked.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         */
		function buttonLink(text: String, action: String, options: Object = null): Void {
            //  TODO - is this right using "buttonLink" as the field?
            options = setOptions("buttonLink", options)
            let connector = getConnector("buttonLink", options)
            connector.buttonLink(text, makeUrl(action, "", options), options)
        }


		/**
		 *	Render a chart. The chart control can display static or dynamic tabular data. The client chart control manages
         *  sorting by column, dynamic data refreshes, pagination and clicking on rows.
         *  @param initialData Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data.
         *  @param options Object Optional extra options. See also $getOptions for a list of the standard options.
         *	@option columns Object hash of column entries. Each column entry is in-turn an object hash of options. If unset, 
         *      all columns are displayed using defaults.
         *  @option kind String Type of chart. Select from: piechart, table, linechart, annotatedtimeline, guage, map, 
         *      motionchart, areachart, intensitymap, imageareachart, barchart, imagebarchart, bioheatmap, columnchart, 
         *      linechart, imagelinechart, imagepiechart, scatterchart (and more)
         *  @option onClick String Action or URL to invoke when a chart element  is clicked.
         *  @example
         *      <% chart(null, { data: "getData", refresh: 2" }) %>
         *      <% chart(data, { onClick: "action" }) %>
		 */
		function chart(initialData: Array, options: Object = null): Void {
            let connector = getConnector("chart", options)
            connector.chart(initialData, options)
		}


        /**
         *  Render an input checkbox. This creates a checkbox suitable for use within an input form. 
         *  @param field Name of the field to display. This is used to create a HTML "name" and "id" attribute for the 
         *      input element. If used inside a model form, it is the field name in the model containing the checkbox
         *      value to display. If used without a model, the value to display should be passed via options.value. 
         *  @param choice Value to submit if checked
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         */
		function checkbox(field: String, choice: String, options: Object = null): Void {
            options = setOptions(field, options)
            let value = getValue(currentModel, field, options)
            let connector = getConnector("checkbox", options)
            connector.checkbox(options.fieldName, value, choice, options)
        }


        /**
         *  End an input form. This closes an input form initiated by calling the $form method.
         */
        function endform(): Void {
            let connector = getConnector("endform", null)
            connector.endform()
            currentModel = undefined
        }


        //  TODO - how does this work if you don't want to associate a model record?
        //  TODO - should the type of the first arg be Record?
        //  TODO - should have an option to not show validation errors
        /*
         *  Render a form.
         *  @param action Action to invoke when the form is submitted. Defaults to "create" or "update" depending on 
         *      whether the field has been previously saved.
         *  @param record Model record to edit
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 *	@option url String Use a URL rather than action and controller for the target url.
         */
        function form(action: String, record: Object = null, options: Object = null): Void {
/* TODO - UNUSED
            if (record == null) {
                record = new Model
            }
*/
            currentModel = record
            formErrors(record)
            options = setOptions("form", options)
            if (options.method == null) {
                options.method = "POST"
            }
            if (action == null) {
                action = "update"
            }
            let connector = getConnector("form", options)
            options.url = makeUrl(action, record.id, options)
            connector.form(record, options.url, options)
        }


        /**
         *  Render an image control
         *  @param src Optional initial source name for the image. The data option may be used with the refresh option to 
         *      dynamically refresh the data.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @examples
         *      <% image("myPic.gif") %>
         *      <% image("myPic.gif", { data: "getData", refresh: 2, style: "myStyle" }) %>
         */
        function image(image: String, options: Object = null): Void {
            let connector = getConnector("image", options)
            connector.image(image, options)
        }


        /**
         *  Render an input field as part of a form. This is a smart input control that will call the appropriate
         *      input control based on the model field data type.
         *  @param field Model field name containing the text data for the control.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @examples
         *      <% input(modelFieldName) %>
         *      <% input(null, { options }) %>
         */
        function input(field: String, options: Object = null): Void {
            datatype = currentModel.getFieldType(field)

            //  TODO - needs fleshing out for each type
            switch (datatype) {
            case "binary":
            case "date":
            case "datetime":
            case "decimal":
            case "float":
            case "integer":
            case "number":
            case "string":
            case "time":
            case "timestamp":
                text(field, options)
                break

            case "text":
                textarea(field, options)
                break

            case "boolean":
                checkbox(field, "true", options)
                break

            default:
                throw "input control: Unknown field type: " + datatype + " for field " + field
            }
        }


        /**
         *  Render a text label field. This renders an output-only text field. Use text() for input fields.
         *  @param text Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @examples
         *      <% label("Hello World") %>
         *      <% label(null, { data: "getData", refresh: 2, style: "myStyle" }) %>
         */
        function label(text: String, options: Object = null): Void {
            options = setOptions("label", options)
            let connector = getConnector("label", options)
            connector.label(text, options)
        }


        //  TODO add method to support RESTful ws
		/** 
		 *	Emit a link to an action. The URL is constructed from the given action and the current controller. The controller
         *  may be overridden by setting the controller option.
         *  @param text Link text to display
         *  @param action Action to invoke when the link is clicked
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 *	@option controller String Name of the target controller for the given action
		 *	@option url String Use a URL rather than action and controller for the target url.
		 */
		function link(text: String, action: String = null, options: Object = null): Void {
            if (action == null) {
                action = text.split(" ")[0].toLower()
            }
            options = setOptions("link", options)
            let connector = getConnector("link", options)
            connector.link(text, makeUrl(action, options.id, options), options)
		}


		/** 
		 *	Emit an application relative link. If invoking an action, it is safer to use \a action.
         *  @param text Link text to display
         *  @param url Action or URL to invoke when the link is clicked
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 */
		function extlink(text: String, url: String, options: Object = null): Void {
            let connector = getConnector("extlink", options)
            connector.extlink(text, controller.appUrl + url, options)
		}


        /*
         *  Emit a selection list. 
         *  @param field Name of the field to display. This is used to create a HTML "name" and "id" attribute for the 
         *      input element. If used inside a model form, it is the field name in the model containing the list item to
         *      select. If used without a model, the value to select should be passed via options.value. 
         *  @param choices Choices to select from. This can be an array list where each element is displayed and the value 
         *      returned is an element index (origin zero). It can also be an array of array tuples where the first 
         *      tuple entry is the value to display and the second is the value to send to the app. Or it can be an 
         *      array of objects such as those returned from a table lookup. If choices is null, the $field value is 
         *      used to construct a model class name to use to return a data grid containing an array of row objects. 
         *      The first non-id field is used as the value to display.
         *  Examples:
         *      list("stockId", Stock.stockList) 
         *      list("low", ["low", "med", "high"])
         *      list("low", [["low", "3"], ["med", "5"], ["high", "9"]])
         *      list("low", [{low: 3{, {med: 5}, {high: 9}])
         *      list("Stock Type")                          Will invoke StockType.findAll() to do a table lookup
         */
		function list(field: String, choices: Object = null, options: Object = null): Void {
            options = setOptions(field, options)
            if (choices == null) {
                //TODO - is this de-pluralizing?
                modelTypeName = field.replace(/\s/, "").toPascal()
                modelTypeName = modelTypeName.replace(/Id$/, "")
                if (global[modelTypeName] == undefined) {
                    throw new Error("Can't find model to create list data: " + modelTypeName)
                }
                choices = global[modelTypeName].findAll()
            }
            let value = getValue(currentModel, field, options)
            let connector = getConnector("list", options)
            connector.list(options.fieldName, choices, value, options)
        }


        /**
         *  Emit a mail link
         *  @param nameText Recipient name to display
         *  @param address Mail recipient address
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         */
		function mail(nameText: String, address: String, options: Object = null): Void  {
            let connector = getConnector("mail", options)
            connector.mail(nameText, address, options)
		}


		/** 
		 *	Emit a progress bar. 
         *  @param initialData Optional initial data for the control. The data option may be used with the refresh option 
         *      to dynamically refresh the data. Progress data is a simple Number in the range 0 to 100 and represents 
         *      a percentage completion value.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @example
         *      <% progress(null, { data: "getData", refresh: 2" }) %>
		 */
		function progress(initialData: Object, options: Object = null): Void {
            let connector = getConnector("progress", options)
            connector.progress(initialData, options)
		}


		/** 
		 *	Emit a radio autton. The URL is constructed from the given action and the current controller. The controller
         *      may be overridden by setting the controller option.
         *  @param field Name of the field to display. This is used to create a HTML "name" and "id" attribute for the 
         *      input element. If used inside a model form, it is the field name in the model containing the radio data to
         *      display. If used without a model, the value to display should be passed via options.value. 
         *  @param choices Array or object containing the option values. If array, each element is a radio option. If an 
         *      object hash, then they property name is the radio text to display and the property value is what is returned.
         *  @param action Action to invoke when the button is clicked or invoked
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 *	@option controller String Name of the target controller for the given action
		 *	@option value String Name of the option to select by default
         *  @example
         *      radio("priority", ["low", "med", "high"])
         *      radio("priority", {low: 0, med: 1, high: 2})
         *      radio(priority, Message.priorities)
		 */
		function radio(field: String, choices: Object, options: Object = null): Void {
            options = setOptions(field, options)
            let value = getValue(currentModel, field, options)
            let connector = getConnector("radio", options)
            connector.radio(options.fieldName, value, choices, options)
        }


		/** 
		 *	Emit a script link.
         *  @param url URL for the script to load
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 */
		function script(url: Object, options: Object = null): Void {
            let connector = getConnector("script", options)
            if (url is Array) {
                for each (u in url) {
                    connector.script(controller.appUrl + "/" + u, options)
                }
            } else {
                connector.script(controller.appUrl + "/" + url, options)
            }
		}


		/** 
		 *	Emit a status control area. 
         *  @param initialData Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data. Status data is a simple String. Status messages may be updated by calling the
         *      \a statusMessage function.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @example
         *      <% status("Initial Status Message", { data: "getData", refresh: 2" }) %>
		 */
		function status(initialData: Object, options: Object = null): Void {
            let connector = getConnector("status", options)
            connector.status(initialData, options)
		}


		/** 
		 *	Emit a style sheet link.
         *  @param url Stylesheet url or array of stylesheets
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
		 */
		function stylesheet(url: Object, options: Object = null): Void {
            let connector = getConnector("stylesheet", options)
            if (url is Array) {
                for each (u in url) {
                    connector.stylesheet(controller.appUrl + "/" + u, options)
                }
            } else {
                connector.stylesheet(controller.appUrl + "/" + url, options)
            }
		}


        /*
         *  TODO table
         *  - in-cell editing
         *  - pagination
         */
		/**
		 *	Render a table. The table control can display static or dynamic tabular data. The client table control manages
         *      sorting by column, dynamic data refreshes, pagination and clicking on rows.
         *  @param data Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data. Table data is an Array of objects where each object represents the data for
         *      a row. The column names are the object property names and the cell text is the object property values.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @option click String Action or URL to invoke when the row is clicked.
         *	@option columns Object hash of column entries. Each column entry is in-turn an object hash of options. If unset, 
         *      all columns are displayed using defaults.
         *  @option pageSize Number Number of rows to display per page. Omit or set to <= 0 for unlimited. 
         *      Defaults to unlimited.
         *  @option pivot Boolean Pivot the table by swaping rows for columns and vice-versa
         *  @option showId Boolean If a columns option is not provided, the id column is normally hidden. 
         *      To display, set showId to be true.
         *  @option sortable Boolean Can the column be sorted on. Defaults to true.
         *  @option sortColumn String Column to sort by. Defaults to first column. This option must be used within a 
         *      column option.
         *  @option sortOrder String Set to "ascending" or "descending". Defaults to ascending. This column must be used 
         *      within a column option.
         *  @option styleHead String CSS style to use for the table header cells
         *  @option styleOddRow String CSS style to use for odd rows in the table
         *  @option styleEvenRow String CSS style to use for even rows in the table
         *  @option title String Table title
         *
         *  Column options: header, width, sort, sortOrder
         *  
         *  @example
         *      <% table(null, { data: "getData", refresh: 2, pivot: true" }) %>
         *      <% table(data, { click: "edit" }) %>
         *      <% table(data, { pivot: true }) %>
         *      <% table(data, {
         *          click: "edit",
         *          columns: {
         *              product:    { header: "Product", width: "20%", sort: "ascending" }
         *              date:       { format: date('%m-%d-%y) }
         *          }
         *       }) %>
		 */
		function table(data: Array, options: Object = null): Void {
            //  TODO - is this right using "table" as the field?
            options = setOptions("table", options)
            let connector = getConnector("table", options)
            if (controller.params.filter) {
                filter(data)
            }
            if (controller.params.sort) {
                sort(data)
            }
            connector.table(data, options)
		}


        /**
         *  Render a tab control. The tab control can display static or dynamic tree data.
         *  @param initialData Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data. Tab data is an array of objects -- one per tab. For example:
                [{"Tab One Label", "action1"}, {"Tab Two Label", "action2"}]
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         */
        function tabs(initialData: Array, options: Object = null): Void {
            let connector = getConnector("tabs", options)
            connector.tabs(initialData, options)
        }


        /**
         *  Render a text input field as part of a form.
         *  @param field Name of the field to display. This is used to create a HTML "name" and "id" attribute for the 
         *      input element. If used inside a model form, it is the field name in the model containing the text data to
         *      display. If used without a model, the value to display should be passed via options.value. 
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *	@option escape Boolean Escape the text before rendering. This converts HTML reserved tags and delimiters into
         *      an encoded form.
         *  @option style String CSS Style to use for the control
		 *	@option visible Boolean Make the control visible. Defaults to true.
         *  @examples
         *      <% text("name") %>
         */
        function text(field: String, options: Object = null): Void {
            options = setOptions(field, options)
            let value = getValue(currentModel, field, options)
            let connector = getConnector("text", options)
            connector.text(options.fieldName, value, options)
        }


//  TODO - need a rich text editor. Wiki style
        /**
         *  Render a text area
         *  @param field Name of the field to display. This is used to create a HTML "name" and "id" attribute for the 
         *      input element. If used inside a model form, it is the field name in the model containing the text data to
         *      display. If used without a model, the value to display should be passed via options.value. 
         *	@option Boolean escape Escape the text before rendering. This converts HTML reserved tags and delimiters into
         *      an encoded form.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
	 	 *	@option data String URL or action to get data 
	 	 *	@option numCols Number number of text columns
	 	 *	@option numRows Number number of text rows
         *  @option style String CSS Style to use for the control
		 *	@option visible Boolean Make the control visible. Defaults to true.
         *  @examples
         *      <% textarea("name") %>
         */
        function textarea(field: String, options: Object = null): Void {
            options = setOptions(field, options)
            let value = getValue(currentModel, field, options)
            let connector = getConnector("textarea", options)
            connector.textarea(options.fieldName, value, options)
        }


        /**
         *  Render a tree control. The tree control can display static or dynamic tree data.
         *  @param initialData Optional initial data for the control. The data option may be used with the refresh option to 
         *      dynamically refresh the data. The tree data is an XML document.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
	 	 *	@option data URL or action to get data 
	 	 *	@option refresh If set, this defines the data refresh period. Only valid if the data option is defined
         *  @option style String CSS Style to use for the control
		 *	@option visible Boolean Make the control visible. Defaults to true.
         */
//  TODO - initial data should be a grid not XML
        function tree(initialData: XML, options: Object = null): Void {
            let connector = getConnector("tree", options)
            connector.tree(initialData, options)
        }


        /*********************************** Wrappers for Control Methods ***********************************/
		/** 
		 *	Emit a flash message area. 
         *  @param kinds Kinds of flash messages to display. May be a single string 
         *      ("error", "inform", "message", "warning"), an array of strings or null. If set to null (or omitted), 
         *      then all flash messages will be displayed.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
	 	 *	@option retain Number. Number of seconds to retain the message. If <= 0, the message is retained until another
         *      message is displayed. Default is 0.
         *  @example
         *      <% flash("status") %>
         *      <% flash() %>
         *      <% flash(["error", "warning"]) %>
		 */
		function flash(kinds = null, options: Object = null): Void {
            //  TODO - is this right using "flash" as the field?
            options = setOptions("flash", options)

            let cflash = controller.flash
            if (cflash == null || cflash.length == 0) {
                return
            }

            let msgs: Object
            if (kinds is String) {
                msgs = {}
                msgs[kinds] = cflash[kinds]

            } else if (kinds is Array) {
                msgs = {}
                for each (kind in kinds) {
                    msgs[kind] = cflash[kind]
                }

            } else {
                msgs = cflash
            }

            for (kind in msgs) {
                let msg: String = msgs[kind]
                if (msg && msg != "") {
                    let connector = getConnector("flash", options)
                    options.style = "flash flash" + kind.toPascal()
                    connector.flash(kind, msg, options)
                }
            }
		}


        //  TODO - how to localize this?
        private function formErrors(model): Void {
            if (!model) {
                return
            }
            let errors = model.getErrors()
            if (errors) {
                write('<div class="formError"><h2>The ' + Reflect(model).name.toLower() + ' has ' + 
                    errors.length + (errors.length > 1 ? ' errors' : ' error') + ' that ' +
                    ((errors.length > 1) ? 'prevent' : 'prevents') + '  it being saved.</h2>\r\n')
                write('    <p>There were problems with the following fields:</p>\r\n')
                write('    <ul>\r\n')
                for (e in errors) {
                    write('        <li>' + e.toPascal() + ' ' + errors[e] + '</li>\r\n')
                }
                write('    </ul>\r\n')
                write('</div>\r\n')
            }
        }


        /*********************************** Wrappers for Control Methods ***********************************/

        /**
         *  Enable session control. This enables session state management for this request and other requests from 
         *  the browser. If a session has not already been created, this call creates a session and sets the @sessionID 
         *  property in the request object. If a session already exists, this call has no effect. A cookie containing a 
         *  session ID is automatically created and sent to the client on the first response after creating the session. 
         *  If SessionAutoCreate is defined in the configuration file, then sessions will automatically be created for 
         *  every web request and your Ejscript web pages do not need to call createSession. Multiple requests may be 
         *  sent from a client's browser at the same time.  Ejscript will ensure that accesses to the sesssion object 
         *  are correctly serialized. 
         *  @param timeout Optional timeout for the session in seconds. If ommitted the default timeout is used.
         */
        function createSession(timeout: Number): Void {
            controller.createSession(timeoout)
        }


        /**
         *  Destroy a session. This call destroys the session state store that is being used for the current client. If no 
         *  session exists, this call has no effect.
         */
        function destroySession(): Void {
            controller.destroySession()
        }


        //  TODO - move non-controls out of this controls-only section
		/** 
		 *	HTML encode the arguments
         *  @param args Variable arguments that will be converted to safe html
         *  @return A string containing the encoded arguments catenated together
		 */
		function html(...args): String {
            return controller.html(args)
		}


        /**
         *  @duplicate Controller.makeUrl
         */
        function makeUrl(action: String, id: String = null, options: Object = null): String {
            return controller.makeUrl(action, id, options)
        }


        /**
         *  Redirect the client to a new URL. This call redirects the client's browser to a new location specified by 
         *  the @url. Optionally, a redirection code may be provided. Normally this code is set to be the HTTP 
         *  code 302 which means a temporary redirect. A 301, permanent redirect code may be explicitly set.
         *  @param url Url to redirect the client to
         *  @param code Optional HTTP redirection code
         */
        function redirectUrl(url: String, code: Number = 302): Void {
            controller.redirectUrl(url, code)
        }


        /**
         *  Redirect to the given action
         *  Options: id controller
         */
        function redirect(action: String, id: String = null, options: Object = null): Void {
            redirectUrl(makeUrl(action, id, options))
        }


        /**
         *  Define a cookie header to include in the reponse
         *  TODO - reconsider arg order
         */
        function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void {
            controller.setCookie(name, value, lifetime, path, secure)
        }


        /**
         *  of the format "keyword: value". If a header has already been defined and \a allowMultiple is false, 
         *  the header will be overwritten. If \a allowMultiple is true, the new header will be appended to the 
         *  response headers and the existing header will also be output. NOTE: case does not matter in the header keyword.
         *  @param header Header string
         *  @param allowMultiple If false, overwrite existing headers with the same keyword. If true, all headers are output.
         */
        function setHeader(key: String, value: String, allowMultiple: Boolean = false): Void {
            controller.setHeader(key, value, allowMultiple)
        }


        /**
         *  Set the HTTP response status code
         *  @param code HTTP response code to define
         */
        function setHttpCode(code: Number): Void {
            controller.setHttpCode(code)
        }


        /**
         *  Set the response body mime type
         *  @param format Mime type for the response. For example "text/plain".
         */
        function setMimeType(format: String): Void {
            controller.setMimeType(format);
        }


        /**
         *  @duplicate Controller.write
         */
		function write(...args): Void {
            controller.write(args)
        }


        /**
         *  Write HTML escaped text to the client. This call writes the arguments back to the client's browser after mapping
         *  all HTML control sequences into safe alternatives. The arguments are converted to strings before writing back to 
         *  the client and then escaped. The text, written using write, will be buffered up to a configurable maximum. This 
         *  allows text to be written prior to setting HTTP headers with setHeader.
         *  @param args Text or objects to write to the client
         */
		function writeHtml(...args): Void {
			controller.write(html(args))
		}


        //  TODO - omit the new line
        /**
         *  @duplicate Controller.writeRaw
         */
        function writeRaw(...args): Void {
            controller.writeRaw(args)
        }


        /**
         *  Dump objects for debugging
         *  @param args List of arguments to print.
         */
        function d(...args): Void {
            write('<pre>\r\n')
            for each (var e: Object in args) {
                write(serialize(e) + "\r\n")
            }
            write('</pre>\r\n')
        }

        /******************************************** Support ***********************************************/

        //  TODO - DOC
        private function addHelper(fun: Function, overwrite: Boolean = false): Void {
            let name: String = Reflect(fun).name
            if (this[name] && !overwrite) {
                throw new Error('Helper ' + name + ' already exists')
            }
            this[name] = fun
        }


        /*
         *  Get the view connector to render a control
         */
		private function getConnector(kind: String, options: Object) {
            var connectorName: String
            if (options && options["connector"]) {
                connectorName = options["connector"]
            } else {
                connectorName =  config.view.connectors[kind]
            }
            if (connectorName == undefined || connectorName == null) {
                connectorName =  config.view.connectors["rest"]
                if (connectorName == undefined || connectorName == null) {
                    connectorName = "html"
                }
                config.view.connectors[kind] = connectorName
            }
            let name: String = (connectorName + "Connector").toPascal()
            try {
                return new global[name]
            } catch (e: Error) {
                throw new Error("Undefined view connector: " + name)
            }
        }


        /*
         *  Update the options based on the model and field being considered
         */
        private function setOptions(field: String, options: Object): Object {
            if (options == null) {
                options = {}
            }
            if (options.fieldName == null) {
                if (currentModel) {
                    options.fieldName = Reflect(currentModel).name.toCamel() + '.' + field
                } else {
                    options.fieldName = field;
                }
            }
            //  TODO - fix IDs this is insufficient
            if (options.id == null) {
                if (currentModel) { 
                    if (currentModel.id) {
                        options.id = field + '_' + currentModel.id
                    }
                }
            }
            if (options.style == null) {
                //  TODO - not sure this is the right thing to do.
                options.style = field
            }
            if (currentModel && currentModel.hasError(field)) {
                options.style += " fieldError"
            }
            return options
        }


        /*
         *  Format the data for presentation. Formats are supplied via (in priority order) from config.view.formats.
         *  If model is supplied, use the specified to get the value. If model is null, then the field is the value to use. 
         *  TODO - opt
         */
        function getValue(model: Object, field: String, options: Object): String {
            let value
            if (model && field) {
                value = model[field]
            }
            if (value == null || value == undefined) {
                if (options.value) {
                    value = options.value
                }
            }
            if (value == null || value == undefined) {
                value = ""
            }
            if (options.render != undefined && options.render is Function) {
                return options.render(value, model, field).toString()
            }
            let typeName = Reflect(value).typeName
            let fmt = config.view.formats[typeName]
            //  TODO - opt
            if (fmt == undefined || fmt == null || fmt == "") {
                return value.toString()
            }
            switch (typeName) {
            case "Date":
                return new Date(value).format(fmt)
            case "Number":
                return fmt.format(value)
            }
            return value.toString()
        }


        /**
         *  Temporary helper function to format the date. TODO
         *  @param fmt Format string
         *  @returns a formatted string
         */
        function date(fmt: String): String {
            return function (data: String): String {
                return new Date(data).format(fmt)
            }
        }


        /**
         *  Temporary helper function to format a number as currency. TODO
         *  @param fmt Format string
         *  @returns a formatted string
         */
        function currency(fmt: String): String {
            return function (data: String): String {
                return fmt.format(data)
            }
        }


        /**
         *  Temporary helper function to format a number. TODO
         *  @param fmt Format string
         *  @returns a formatted string
         */
        function number(fmt: String): String {
            return function (data: String): String {
                return fmt.format(data)
            }
        }


        /*
         *  Mapping of helper options to HTML attributes ("" value means don't map the name)
         */
        private static const htmlOptions: Object = { 
            background: "", color: "", id: "", height: "", method: "", size: "", 
            style: "class", visible: "", width: "",
        }


        /**
         *  Map options to a HTML attribute string.
         *  @param options Optional extra options. See $getOptions for a list of the standard options.
         *  @returns a string containing the HTML attributes to emit.
	 	 *	@option background String Background color. This is a CSS RGB color specification. For example "FF0000" for red.
	 	 *	@option color String Foreground color. This is a CSS RGB color specification. For example "FF0000" for red.
	 	 *	@option data String String URL or action to get data for dynamic controls that support live refresh.
         *  @option id String Browser element ID for the control
         *	@option escape Boolean Escape the text before rendering. This converts HTML reserved tags and delimiters into
         *      an encoded form.
		 *	@option height (Number|String) Height of the table. Can be a number of pixels or a percentage string. 
         *	    Defaults to unlimited.
	 	 *	@option method String HTTP method to invoke. May be: GET, POST, PUT or DELETE.
	 	 *	@option refresh Boolean Defines the data refresh period for dynamic controls. Only valid if the data option 
         *	    is defined.
         *  @option size (Number|String) Size of the element.
         *  @option style String CSS Style to use for the control.
		 *	@option value Default data value to use for the control if not supplied by other means.
		 *	@option visible Boolean Make the control visible. Defaults to true.
		 *	@option width (Number|String) Width of the table or column. Can be a number of pixels or a percentage string. 
         */
		function getOptions(options: Object): String {
			if (!options) {
                //  TODO - should this have a trailing space?
                return ''
            }
            let result: String = ""
            for (let option: String in options) {
                let mapped = htmlOptions[option]
                if (mapped) {
                    if (mapped == "") {
                        /* No mapping, keep the original option name */
                        mapped = option
                    }
                    result += ' ' +  mapped + '="' + options[option] + '"'
                }
            }
            return result + " "
		}


        //  TODO - not implemented
/*
        private function sortFn(a: Array, ind1: Number, ind2: Number) {
            if (a < b) {
                return -1
            } else if (a > b) {
                return 1
            }
            return 0
        }
*/


        private function sort(data: Array) {
/*
            data["sortBy"] = controller.params.sort
            data["sortOrder"] = controller.params.sortOrder
            data.sort(sortFn)
*/
            let tmp = data[0]
            data[0] = data[1]
            data[1] = tmp
        }


        private function filter(data: Array) {
            pattern = controller.params.filter.toLower()
            for (let i = 0; i < data.length; i++) {
                let found: Boolean = false
                for each (f in data[i]) {
                    if (f.toString().toLower().indexOf(pattern) >= 0) {
                        found = true
                    }
                }
                if (!found) {
                    data.remove(i, i)
                    i--
                }
            }
        }
	}

    internal class Model implements Record {
        setup()
        function Model(fields: Object = null) {
            constructor(fields)
        }
    }
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
/************************************************************************/
/*
 *  End of file "../es/web/View.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/xml/XML.es"
 */
/************************************************************************/

/*
 *	XML.es - XML class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.

Samples:

	order = <{x}>{item}</{x}>
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The XML class, and the entire XML API, is based on ECMA-357, ECMAScript for XML (E4X). The XML class is a 
	 *	core class in the E4X specification; it provides the ability to load, parse and save XML documents.
	 *	@spec ecma-357
	 */
	native final class XML extends Object {

        use default namespace public

		/**
		 *	XML Constructor. Create an empty XML object.
		 *	@param value An optional XML or XMLList object to clone.
		 *	@return An XML node object.
		 */
		native function XML(value: Object = null)

		/**
		 */
		native function load(filename: String): Void

		/**
		 */
		native function save(filename: String): Void

		/**
		 *	TODO - doc
		 */
		/* # ECMA */
		static var ignoreComments: Boolean

		/* # ECMA */
		static var ignoreProcessingInstructions: Boolean

		/* # ECMA */
		static var ignoreWhitespace: Boolean

		/* # ECMA */
		static var prettyPrinting: Boolean

		/* # ECMA */
		static var prettyIndent: Boolean

//	TODO - all these methods should be in the prototype namespace (prototype
//	object in the spec) or some other namespace. Method lookup must NOT find them unless doing a call()

		/* # ECMA */
		native function addNamespace(ns: Namespace): XML


		/**
		 *	Append a child to this XML object.
		 *	@param child The child to add.
		 *	@return This object with the added child.
		 */
		/* # ECMA */
		native function appendChild(child: XML): XML


		/**
		 *	Get an XMLList containing all of the attributes of this object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		/* # ECMA */
		native function attribute(name: String): XMLList


		/**
		 *	Get an XMLList containing all of the attributes of this object.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		/* # ECMA */
		native function attributes(): XMLList
		

		/**
		 *	Get an XMLList containing the list of children (properties) in this XML object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the children names (zero or more).
		 */
		/* # ECMA */
		native function child(name: String): XMLList
		

		/**
		 *	Get the position of this object within the context of its parent.
		 *	@return A number with the zero-based position.
		 */
		/* # ECMA */
		native function childIndex(): Number
		

		/**
		 *	Get an XMLList containing the children (properties) of this object in order.
		 *	@return An XMLList with all the properties.
		 */
		/* # ECMA */
		native function children(): XMLList
		

		/**
		 *	Get an XMLList containing the properties of this object that are
		 *	comments.
		 *	@return An XMLList with all the comment properties.
		 */
		/* # ECMA */
		native function comments(): XMLList
		

		/**
		 *	Compare an XML object to another one or an XMLList with only one
		 *	XML object in it. If the comparison operator is true, then one
		 *	object is said to contain the other.
		 *	@return True if this object contains the argument.
		 */
		/* # ECMA */
		native function contains(obj: Object): Boolean
		

		/**
		 *	Deep copy an XML object. The new object has its parent set to null.
		 *	@return Then new XML object.
		 */
		/* # ECMA */
		native function copy(): XML
		

		/**
		 *	Get the defaults settings for an XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		/* # ECMA */
		native function defaultSettings(): Object
		

		/**
		 *	Get all the descendants (that have the same name) of this XML object. The optional argument defaults 
		 *	to getting all descendants.
		 *	@param name The (optional) name to search on.
		 *	@return The list of descendants.
		 */
		/* # ECMA */
		native function descendants(name: String = "*"): Object
		

		/**
		 *	Get all the children of this XML node that are elements having the
		 *	given name. The optional argument defaults to getting all elements.
		 *	@param name The (optional) name to search on.
		 *	@return The list of elements.
		 */
		/* # ECMA */
		native function elements(name: String = "*"): XMLList
		

		/**
		 *	Get an iterator for this node to be used by "for (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this node to be used by "for each (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator

		/**
		 *	Determine whether this XML object has complex content. If the object has child elements it is 
		 *	considered complex.
		 *	@return True if this object has complex content.
		 */
		/* # ECMA */
		native function hasComplexContent(): Boolean
		

		/**
		 *	Determine whether this object has its own property of the given name.
		 *	@param prop The property to look for.
		 *	@return True if this object does have that property.
		 */
		# ECMA
		override intrinsic native function hasOwnProperty(name: String): Boolean
		

		/**
		 *	Determine whether this XML object has simple content. If the object
		 *	is a text node, an attribute node or an XML element that has no
		 *	children it is considered simple.
		 *	@return True if this object has simple content.
		 */
		/* # ECMA */
		native function hasSimpleContent(): Boolean
		

		/* # ECMA */
		native function inScopeNamespaces(): Array


		/**
		 *	Insert a child object into an XML object immediately after a specified marker object. If the marker object 
		 *	is null then the new object is inserted at the end. If the marker object is not found then the insertion 
		 *	is not made.
		 *	TODO - if marker is null. Insert at beginning or end?
		 *	@param marker Insert the new element before this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		/* # ECMA */
		native function insertChildAfter(marker: Object, child: Object): XML
		

		/**
		 *	Insert a child object into an XML object immediately before a specified marker object. If the marker 
		 *	object is null then the new object is inserted at the end. If the marker object is not found then the
		 *	insertion is not made.
		 *	TODO - if marker is null. Insert at beginning or end?
		 *	@param marker Insert the new element before this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		/* # ECMA */
		native function insertChildBefore(marker: Object, child: Object): XML
		

		/**
		 *	Return the length of an XML object; the length is defined as 1.
		 *	@return 1.
		 */
		override native function length(): Number
		

		/**
		 *	Get the local name portion of the complete name of this XML object.
		 *	@return The local name.
		 */
		/* # ECMA */
		native function localName(): String
		

		/**
		 *	Get the qualified name of this XML object.
		 *	@return The qualified name.
		 */
		native function name(): String
		

		/* # ECMA */
		native function namespace(prefix: String = null): Object


		/* # ECMA */
		native function namespaceDeclarations(): Array


		/**
		 *	Get the kind of node this XML object is.
		 *	@return The node kind.
		 */
		/* # ECMA */
		native function nodeKind(): String
		

		/**
		 *	Merge adjacent text nodes into one text node and eliminate empty text nodes.
		 *	@return This XML object.
		 */
		/* # ECMA */
		native function normalize(): XML
		

		/**
		 *	Get the parent of this XML object.
		 *	@return The parent.
		 */
		/* # ECMA */
		native function parent(): XML
		

		/**
		 *	Insert a child object into an XML object immediately before all existing properties.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		/* # ECMA */
		native function prependChild(child: Object): XML
		

		/**
		 *	Get all the children of this XML node that are processing instructions having the given name. 
		 *	The optional argument defaults to getting all processing instruction nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of processing instruction nodes.
		 */
		/* # ECMA */
		native function processingInstructions(name: String = "*"): XMLList
		

		/**
		 *	Test a property to see if it will be included in an enumeration over the XML object.
		 *	@param property The property to test.
		 *	@return True if the property will be enumerated.
		 */
		# ECMA
		override intrinsic native function propertyIsEnumerable(property: Object): Boolean
		

		/**
		 *	Change the value of a property. If the property is not found, nothing happens.
		 *	@param property The property to change.
		 *	@param value The new value.
		 *	@return True if the property will be enumerated.
		 */
		/* # ECMA */
		native function replace(property: Object, value: Object): void

		
		/**
		 *	Replace all the current properties of this XML object with a new set. The argument can be 
		 *	another XML object or an XMLList.
		 *	@param properties The new property or properties.
		 *	@return This XML object.
		 */
		/* # ECMA */
		native function setChildren(properties: Object): XML
		

		/**
		 *	Set the local name portion of the complete name of this XML object.
		 *	@param The local name.
		 */
		/* # ECMA */
		native function setLocalName(name: String): void
		

		/**
		 *	Set the qualified name of this XML object.
		 *	@param The qualified name.
		 */
		/* # ECMA */
		native function setName(name: String): void
		

		/**
		 *	Get the settings for this XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		/* # ECMA */
		native function settings(): Object
		

		/**
		 *	Configure the settings for this XML object.
		 *	@param settings A "settings" object previously returned from a call to the "settings" method.
		 */
		/* # ECMA */
		native function setSettings(settings: Object): void

		
		/**
		 *	Get all the properties of this XML node that are text nodes having the given name. The optional 
		*	argument defaults to getting all text nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of text nodes.
		 */
		/* # ECMA */
		native function text(name: String = "*"): XMLList
		

		/**
		 *	Provides an XML-encoded version of the XML object that includes the tags.
		 *	@return A string with the encoding.
		 */
		/* # ECMA */
		native function toXMLString(): String 
		

		/**
		 *	Provides a string representation of the XML object.
		 *	@return A string with the encoding.
		 */
		override native function toString(): String 
		

		/**
		 *	Return this XML object.
		 *	@return This object.
		 */
		# ECMA 
		override intrinsic function valueOf(): XML {
			return this
		}
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 */
/************************************************************************/
/*
 *  End of file "../es/xml/XML.es"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../es/xml/XMLList.es"
 */
/************************************************************************/

/*
 *	XMLList.es - XMLList class

 *	Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The XMLList class, and the entire XML API, is based on ECMA-357, ECMAScript for XML (E4X). An XMLList object 
	 *	stores a list of XML objects. If the list has only one element, it functions similar to an XML object.
	 *	@spec ecma-357
	 */
	native final class XMLList extends Object {

        use default namespace public

		/**
		 *	XML Constructor. Create an empty XML object.
		 *	@return An XML node object.
		 */
		native function XMLList() 


		# ASC
		native function addNamespace(ns: Namespace): XML


		/**
		 *	Append a child to this XML object.
		 *	@param child The child to add.
		 *	@return This object with the added child.
		 */
		native function appendChild(child: XML): XML


		/**
		 *	Get an XMLList containing all of the attributes of this object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		native function attribute(name: String): XMLList


		/**
		 *	Get an XMLList containing all of the attributes of this object.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		native function attributes(): XMLList
		

		/**
		 *	Get an XMLList containing the list of children (properties) in this XML object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the children names (zero or more).
		 */
		native function child(name: String): XMLList
		

		/**
		 *	Get the position of this object within the context of its parent.
		 *	@return A number with the zero-based position.
		 */
		native function childIndex(): Number
		

		/**
		 *	Get an XMLList containing the children (properties) of this object in order.
		 *	@return An XMLList with all the properties.
		 */
		native function children(): XMLList
		

		/**
		 *	Get an XMLList containing the properties of this object that are
		 *	comments.
		 *	@return An XMLList with all the comment properties.
		 */
		native function comments(): XMLList
		

		/**
		 *	Compare an XML object to another one or an XMLList with only one XML object in it. If the 
		 *	comparison operator is true, then one object is said to contain the other.
		 *	@return True if this object contains the argument.
		 */
		native function contains(obj: Object): Boolean
		

		/**
		 *	Deep copy an XML object. The new object has its parent set to null.
		 *	@return Then new XML object.
		 */
		native function copy(): XML
		

		/**
		 *	Get the defaults settings for an XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		native function defaultSettings(): Object
		

		/**
		 *	Get all the descendants (that have the same name) of this XML object. The optional argument defaults 
		 *	to getting all descendants.
		 *	@param name The (optional) name to search on.
		 *	@return The list of descendants.
		 */
		native function descendants(name: String = "*"): Object
		

		/**
		 *	Get all the children of this XML node that are elements having the
		 *	given name. The optional argument defaults to getting all elements.
		 *	@param name The (optional) name to search on.
		 *	@return The list of elements.
		 */
		native function elements(name: String = "*"): XMLList
		

		/**
		 *	Get an iterator for this node to be used by "for (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this node to be used by "for each (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator

		/**
		 *	Determine whether this XML object has complex content. If the object has child elements it is 
		 *	considered complex.
		 *	@return True if this object has complex content.
		 */
		native function hasComplexContent(): Boolean
		

		/**
		 *	Determine whether this object has its own property of the given name.
		 *	@param prop The property to look for.
		 *	@return True if this object does have that property.
		 */
		# ECMA
		override intrinsic native function hasOwnProperty(name: String): Boolean
		

		/**
		 *	Determine whether this XML object has simple content. If the object
		 *	is a text node, an attribute node or an XML element that has no
		 *	children it is considered simple.
		 *	@return True if this object has simple content.
		 */
		native function hasSimpleContent(): Boolean
		

		# ASC
		/**
		 *	Return the namespace in scope
		 *	@return Array of namespaces
		 */
		native function inScopeNamespaces(): Array


		/**
		 *	Insert a child object into an XML object immediately after a specified marker object. If the marker 
		 *	object is null then the new object is inserted at the beginning. If the marker object is not found 
		 *	then the insertion is not made.
		 *	TODO - if marker is null, should it insert at the beginning or end?
		 *	@param marker Insert the new element after this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function insertChildAfter(marker: Object, child: Object): XML
		

		/**
		 *	Insert a child object into an XML object immediately before a specified marker object. If the marker 
		 *	object is null then the new object is inserted at the end. If the marker object is not found then the
		 *	insertion is not made.
		 *	TODO - if marker is null, should it insert at the beginning?
		 *	@param marker Insert the new element before this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function insertChildBefore(marker: Object, child: Object): XML
		

		/**
		 *	Return the length of an XML object; the length is defined as 1.
		 *	@return 1.
		 */
		override native function length(): Number
		

		/**
		 *	Get the local name portion of the complete name of this XML object.
		 *	@return The local name.
		 */
		native function localName(): String
		

		/**
		 *	Get the qualified name of this XML object.
		 *	@return The qualified name.
		 */
		native function name(): String
		

		# ASC
		native function namespace(prefix: String = null): Object


		# ASC
		native function namespaceDeclarations(): Array


		/**
		 *	Get the kind of node this XML object is.
		 *	@return The node kind.
		 */
		native function nodeKind(): String
		

		/**
		 *	Merge adjacent text nodes into one text node and eliminate empty text nodes.
		 *	@return This XML object.
		 */
		native function normalize(): XML
		

		/**
		 *	Get the parent of this XML object.
		 *	@return The parent.
		 */
		native function parent(): XML
		

		/**
		 *	Insert a child object into an XML object immediately before all existing properties.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function prependChild(child: Object): XML
		

		/**
		 *	Get all the children of this XML node that are processing instructions having the given name. 
		 *	The optional argument defaults to getting all processing instruction nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of processing instruction nodes.
		 */
		native function processingInstructions(name: String = "*"): XMLList
		

		/**
		 *	Test a property to see if it will be included in an enumeration over the XML object.
		 *	@param property The property to test.
		 *	@return True if the property will be enumerated.
		 */
		# ECMA
		override intrinsic native function propertyIsEnumerable(property: Object): Boolean
		

		/**
		 *	Change the value of a property. If the property is not found, nothing happens.
		 *	@param property The property to change.
		 *	@param value The new value.
		 *	@return True if the property will be enumerated.
		 */
		native function replace(property: Object, value: Object): void

		
		/**
		 *	Replace all the current properties of this XML object with a new set. The argument can be 
		 *	another XML object or an XMLList.
		 *	@param properties The new property or properties.
		 *	@return This XML object.
		 */
		native function setChildren(properties: Object): XML
		

		/**
		 *	Set the local name portion of the complete name of this XML object.
		 *	@param The local name.
		 */
		native function setLocalName(name: String): void
		

		/**
		 *	Set the qualified name of this XML object.
		 *	@param The qualified name.
		 */
		native function setName(name: String): void
		

		/**
		 *	Get the settings for this XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		native function settings(): Object
		

		/**
		 *	Configure the settings for this XML object.
		 *	@param settings A "settings" object previously returned from a call to the "settings" method.
		 */
		native function setSettings(settings: Object): void

		
		/**
		 *	Get all the properties of this XML node that are text nodes having the given name. The optional 
		*	argument defaults to getting all text nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of text nodes.
		 */
		native function text(name: String = "*"): XMLList
		

		/**
		 *	Provides a string representation of the XML object.
		 *	@return A string with the encoding.
		 */
		override native function toString(): String 
		

		/**
		 *	Provides an XML-encoded version of the XML object that includes the tags.
		 *	@return A string with the encoding.
		 */
		native function toXMLString(): String 
		

		/**
		 *	Return this XML object.
		 *	@return This object.
		 */
		# ECMA
		override intrinsic function valueOf(): XML {
			return this
		}

		/*
		   XML
		   		.prototype
		   		.ignoreComments
		   		.ignoreProcessingInstructions
		   		.ignoreWhitespace
		   		.prettyPrinting
		   		.prettyIndent
		   		.settings()
		   		.setSettings(settings)
		   		.defaultSettings()

			   function domNode()
			   function domNodeList()
			   function xpath(XPathExpression)
		   XMLList
			   function domNode()
			   function domNodeList()
			   function xpath(XPathExpression)
		 */
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */

/************************************************************************/
/*
 *  End of file "../es/xml/XMLList.es"
 */
/************************************************************************/

