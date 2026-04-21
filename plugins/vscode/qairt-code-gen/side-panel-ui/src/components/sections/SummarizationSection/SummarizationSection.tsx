// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

interface SummarizationSectionProps {
  quoteStyle: string;
}

export function SummarizationSection({ quoteStyle }: SummarizationSectionProps): JSX.Element {
  return (
    <section className="summarization-section">
      <h3>Summarization</h3>
      <span>
        To use summarization for docstrings, start typing docstring quotes (<code>{quoteStyle}</code>).
      </span>
    </section>
  );
}
