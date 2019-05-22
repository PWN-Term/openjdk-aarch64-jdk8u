/*
 * Copyright (c) 2019, Red Hat, Inc. All rights reserved.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_GC_SHENANDOAH_SHENANDOAHROOTPROCESSOR_INLINE_HPP
#define SHARE_GC_SHENANDOAH_SHENANDOAHROOTPROCESSOR_INLINE_HPP

#include "gc_implementation/shenandoah/shenandoahPhaseTimings.hpp"
#include "gc_implementation/shenandoah/shenandoahRootProcessor.hpp"
#include "gc_implementation/shenandoah/shenandoahUtils.hpp"
#include "memory/resourceArea.hpp"

template <typename ITR>
void ShenandoahCodeCacheRoots<ITR>::code_blobs_do(CodeBlobClosure* blob_cl, uint worker_id) {
  ShenandoahWorkerTimingsTracker timer(ShenandoahPhaseTimings::CodeCacheRoots, worker_id);
  _coderoots_iterator.possibly_parallel_blobs_do(blob_cl);
}

template <typename ITR>
ShenandoahRootScanner<ITR>::ShenandoahRootScanner(ShenandoahPhaseTimings::Phase phase) :
  ShenandoahRootProcessor(phase) {
}

template <typename ITR>
void ShenandoahRootScanner<ITR>::roots_do(uint worker_id, OopClosure* oops) {
  CLDToOopClosure clds_cl(oops);
  MarkingCodeBlobClosure blobs_cl(oops, !CodeBlobToOopClosure::FixRelocations);
  roots_do(worker_id, oops, &clds_cl, &blobs_cl);
}

template <typename ITR>
void ShenandoahRootScanner<ITR>::strong_roots_do(uint worker_id, OopClosure* oops) {
  CLDToOopClosure clds_cl(oops);
  MarkingCodeBlobClosure blobs_cl(oops, !CodeBlobToOopClosure::FixRelocations);
  strong_roots_do(worker_id, oops, &clds_cl, &blobs_cl);
}

template <typename ITR>
void ShenandoahRootScanner<ITR>::roots_do(uint worker_id, OopClosure* oops, CLDClosure* clds, CodeBlobClosure* code) {
  assert(!ShenandoahHeap::heap()->unload_classes(),
         "No class unloading");
  ResourceMark rm;
  _serial_roots.oops_do(oops, worker_id);
  _dict_roots.oops_do(oops, worker_id);
  _thread_roots.oops_do(oops, clds, code, worker_id);
  _cld_roots.clds_do(clds, clds, worker_id);

  // With ShenandoahConcurrentScanCodeRoots, we avoid scanning the entire code cache here,
  // and instead do that in concurrent phase under the relevant lock. This saves init mark
  // pause time.
  if (code != NULL && !ShenandoahConcurrentScanCodeRoots) {
    _code_roots.code_blobs_do(code, worker_id);
  }

  _weak_roots.oops_do(oops, worker_id);
  _string_table_roots.oops_do(oops, worker_id);
  _dedup_roots.oops_do(oops, worker_id);
}

template <typename ITR>
void ShenandoahRootScanner<ITR>::strong_roots_do(uint worker_id, OopClosure* oops, CLDClosure* clds, CodeBlobClosure* code) {
  assert(ShenandoahHeap::heap()->unload_classes(), "Should be used during class unloading");
  ResourceMark rm;

  _serial_roots.oops_do(oops, worker_id);
  _dict_roots.strong_oops_do(oops, worker_id);
  _cld_roots.clds_do(clds, NULL, worker_id);
  _thread_roots.oops_do(oops, clds, code, worker_id);
}
#endif // SHARE_GC_SHENANDOAH_SHENANDOAHROOTPROCESSOR_INLINE_HPP
